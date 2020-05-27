#pragma once

#include <fetchpp/core/basic_transport.hpp>
#include <fetchpp/core/detail/coroutine.hpp>
#include <fetchpp/core/process_one.hpp>
#include <fetchpp/core/ssl_transport.hpp>

#include <fetchpp/core/detail/async_http_result.hpp>
#include <fetchpp/core/detail/session_work_queue.hpp>

#include <boost/asio/compose.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/error.hpp>

#include <fetchpp/alias/beast.hpp>
#include <fetchpp/alias/error_code.hpp>
#include <fetchpp/alias/net.hpp>
#include <fetchpp/alias/tcp.hpp>

#include <string_view>

namespace fetchpp
{
namespace detail
{
template <typename Transport, typename Request, typename Response>
struct process_queue_op
{
  base_endpoint& endpoint;
  Transport& transport;
  session_work_queue& pending;
  Request& request;
  Response& response;
  net::coroutine coro_ = {};

  template <typename Self>
  void operator()(Self& self, error_code ec = {})
  {
    FETCHPP_REENTER(coro_)
    {
      if (!beast::get_lowest_layer(transport).socket().is_open() ||
          ec == net::error::eof || ec == net::error::connection_reset)
      {
        FETCHPP_YIELD transport.async_connect(endpoint, std::move(self));
        if (ec)
        {
          self.complete(ec);
          return;
        }
      }
      FETCHPP_YIELD async_process_one(
          transport, request, response, std::move(self));
      pending.consume();
      self.complete(ec);
    }
  }
};

template <typename Transport,
          typename Request,
          typename Response,
          typename Handler>
auto make_work(base_endpoint& endpoint,
               Transport& transport,
               Request& request,
               Response& response,
               Handler&& handler)
{
  struct session_work : detail::session_work_queue::work
  {
    base_endpoint& endpoint;
    Transport& transport;
    Request& request;
    Response& response;
    Handler handler;

    session_work(base_endpoint& e,
                 Transport& d,
                 Request& req,
                 Response& res,
                 Handler&& h)
      : endpoint(e),
        transport(d),
        request(req),
        response(res),
        handler(std::move(h))
    {
    }

    void operator()(session_work_queue& q) override
    {
      return net::async_compose<Handler, void(error_code)>(
          detail::process_queue_op<Transport, Request, Response>{
              endpoint, transport, q, request, response},
          handler,
          transport);
    }
  };
  // FIXME: use someone associated_allocator
  return std::make_unique<session_work>(
      endpoint, transport, request, response, std::move(handler));
}
}

template <typename Endpoint, typename AsyncTransport>
class session
{
public:
  session(session const&) = delete;
  session& operator=(session const&) = delete;

  session(session&&) = delete;
  session& operator=(session&&) = delete;

  using transport_type = AsyncTransport;
  using next_layer_type = typename transport_type::next_layer_type;
  using executor_type = typename transport_type::executor_type;
  using buffer_type = typename transport_type::buffer_type;
  using endpoint_type = Endpoint;

  template <typename... Args>
  session(endpoint_type endpoint, Args&&... args)
    : endpoint_(std::move(endpoint)), transport_(std::forward<Args>(args)...)
  {
  }

  session(endpoint_type endpoint, net::executor& ex)
    : endpoint_(std::move(endpoint)), transport_(ex)
  {
  }

  template <typename CompletionToken>
  auto async_start(CompletionToken&& token)
  {
    return transport_.async_connect(endpoint_,
                                    std::forward<CompletionToken>(token));
  }

  template <typename Response, typename Request, typename CompletionToken>
  auto push_request(Request& req, Response& res, CompletionToken&& token)
  {
    // FIXME: move that to the session strand
    net::async_completion<CompletionToken, void(error_code)> comp{token};
    req.keep_alive(true);
    pending_.push(detail::make_work(
        endpoint_, transport_, req, res, std::move(comp.completion_handler)));
    return comp.result.get();
  }

  auto pending_requests() const
  {
    return pending_.size();
  }

  auto const& endpoint() const
  {
    return endpoint_;
  }

  auto get_executor()
  {
    return transport_.get_executor();
  }

  template <typename CompletionToken>
  auto async_stop(CompletionToken&& token)
  {
    return async_close(transport_, std::forward<CompletionToken>(token));
  }

private:
  endpoint_type endpoint_;
  transport_type transport_;
  detail::session_work_queue pending_;
};

session(secure_endpoint, beast::ssl_stream<beast::tcp_stream> &&)
    ->session<secure_endpoint, ssl_async_transport>;

template <typename AsyncStream>
session(plain_endpoint, AsyncStream &&)
    ->session<plain_endpoint,
              basic_async_transport<AsyncStream, beast::multi_buffer>>;
}
