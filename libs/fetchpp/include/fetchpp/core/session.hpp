#pragma once

#include <fetchpp/core/basic_transport.hpp>
#include <fetchpp/core/detail/coroutine.hpp>
#include <fetchpp/core/process_one.hpp>

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
template <typename AsyncTransport, typename Request, typename Response>
struct session_execute_op
{
  AsyncTransport& transport;
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
        FETCHPP_YIELD async_connect(transport, std::move(self));
        if (ec)
        {
          self.complete(ec);
          return;
        }
      }
      FETCHPP_YIELD async_process_one(
          transport, request, response, std::move(self));
      self.complete(ec);
    }
  }
};
template <typename AsyncTransport, typename Request, typename Response>
session_execute_op(AsyncTransport&, Request&, Response &&)
    ->session_execute_op<AsyncTransport, Request, Response>;

template <typename AsyncTransport,
          typename Request,
          typename Response,
          typename CompletionToken>
auto run_execute(AsyncTransport& transport,
                 Request& req,
                 Response& res,
                 CompletionToken&& token)
{
  return net::async_compose<CompletionToken, void(error_code)>(
      session_execute_op{transport, req, res}, token, transport);
}

template <typename Transport,
          typename Queue,
          typename Request,
          typename Response,
          typename Handler>
struct process_queue_op
{
  Transport& transport;
  Queue& pending;
  Request& request;
  Response& response;
  net::coroutine coro_ = {};

  template <typename Self>
  void operator()(Self& self, error_code ec = {})
  {
    FETCHPP_REENTER(coro_)
    {
      FETCHPP_YIELD run_execute(transport, request, response, std::move(self));
      pending.consume();
      self.complete(ec);
    }
  }
};

template <typename Transport,
          typename Queue,
          typename Request,
          typename Response,
          typename Handler>
auto process_queue(Transport& transport,
                   Queue& queue,
                   Request& request,
                   Response& response,
                   Handler&& handler)
{
  return net::async_compose<Handler, void(error_code)>(
      detail::process_queue_op<Transport, Queue, Request, Response, Handler>{
          transport, queue, request, response},
      handler,
      transport);
}

template <typename Transport,
          typename Request,
          typename Response,
          typename Handler>
auto make_work(Transport& transport,
               Request& request,
               Response& response,
               Handler&& handler)
{
  struct session_work : detail::session_work_queue::work
  {
    Transport& transport;
    Request& request;
    Response& response;
    Handler handler;

    session_work(Transport& d, Request& req, Response& res, Handler&& h)
      : transport(d), request(req), response(res), handler(std::move(h))
    {
    }

    void operator()(session_work_queue& q) override
    {
      process_queue(transport, q, request, response, std::move(handler));
    }
  };
  // FIXME: use someone associated_allocator
  return std::make_unique<session_work>(
      transport, request, response, std::move(handler));
}
}

template <typename AsyncTransport>
class session
{
public:
  session(session const&) = delete;
  session& operator=(session const&) = delete;

  session(session&&) = delete;
  session& operator=(session&&) = delete;

  using next_layer_type = typename AsyncTransport::next_layer_type;
  using executor_type = typename AsyncTransport::executor_type;
  using buffer_type = typename AsyncTransport::buffer_type;

  template <typename... Args>
  session(std::string domain, std::uint16_t port, Args&&... args)
    : transport_(std::move(domain), port, std::forward<Args>(args)...)
  {
  }

  session(std::string domain, std::uint16_t port, net::executor& ex)
    : transport_(std::move(domain), port, ex)
  {
  }

  session(AsyncTransport transport) : transport_(std::move(transport))
  {
  }

  template <typename CompletionToken>
  auto async_start(CompletionToken&& token)
  {
    return async_connect(transport_, std::forward<CompletionToken>(token));
  }

  template <typename Response, typename Request, typename CompletionToken>
  auto push_request(Request& req, Response& res, CompletionToken&& token)
  {
    // FIXME: move that to the session strand
    net::async_completion<CompletionToken, void(error_code)> comp{token};
    req.keep_alive(true);
    pending_.push(detail::make_work(
        transport_,
        req,
        res,
        net::bind_executor(transport_.get_executor(),
                           std::move(comp.completion_handler))));
    return comp.result.get();
  }

  auto pending_requests() const
  {
    return pending_.size();
  }

  auto const& host() const
  {
    return transport_.host();
  }

  auto const& domain() const
  {
    return transport_.domain();
  }

  auto port() const
  {
    return transport_.port();
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
  AsyncTransport transport_;
  detail::session_work_queue pending_;
};
template <typename AsyncStream>
session(std::string, std::uint16_t, AsyncStream &&)
    ->session<basic_async_transport<AsyncStream, beast::multi_buffer>>;
}
