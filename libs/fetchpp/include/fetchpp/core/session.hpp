#pragma once

#include <fetchpp/core/basic_transport.hpp>
#include <fetchpp/core/detail/coroutine.hpp>
#include <fetchpp/core/process_one.hpp>
#include <fetchpp/core/ssl_transport.hpp>
#include <fetchpp/core/tcp_transport.hpp>

#include <fetchpp/core/detail/async_http_result.hpp>
#include <fetchpp/core/detail/session_work_queue.hpp>

#include <boost/asio/compose.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/error.hpp>

#include <fetchpp/alias/beast.hpp>
#include <fetchpp/alias/error_code.hpp>
#include <fetchpp/alias/net.hpp>
#include <fetchpp/alias/tcp.hpp>

#include <chrono>
#include <string_view>

namespace fetchpp
{
namespace detail
{
namespace
{
bool is_brutally_closed(error_code ec)
{
  return ec == net::error::eof || ec == net::error::connection_reset ||
         ec == net::ssl::error::stream_truncated;
}
template <typename NextLayer>
bool is_open(NextLayer const& layer)
{
  return beast::get_lowest_layer(layer).socket().is_open();
}
}

template <typename Session, typename Request, typename Response>
struct process_queue_op
{
  Session& session_;
  Request& request_;
  Response& response_;
  error_code last_ec = {};
  net::coroutine coro_ = {};

  template <typename Self>
  void operator()(Self& self, error_code ec = {})
  {
    FETCHPP_REENTER(coro_)
    {
      if (!is_open(session_.transport_))
      {
        FETCHPP_YIELD session_.async_start(std::move(self));
        if (ec)
        {
          self.complete(ec);
          return;
        }
      }
      FETCHPP_YIELD async_process_one(
          session_.transport_, request_, response_, std::move(self));
      if (ec && ec != beast::error::timeout)
      {
        last_ec = ec;
        FETCHPP_YIELD session_.async_stop(std::move(self));
        ec = last_ec;
      }

      if (is_brutally_closed(ec) && !session_.first_round_)
      {
        // we reset the coroutine state to try a re-connection
        coro_ = {};
        session_.first_round_ = true;
        net::post(beast::bind_front_handler(std::move(self), ec));
        return;
      }
      else
        session_.first_round_ = false;
      session_.work_queue_.consume();
      self.complete(ec);
    }
  }
};

template <typename Session,
          typename Request,
          typename Response,
          typename Handler>
auto make_work(Session& session,
               Request& request,
               Response& response,
               Handler&& handler)
{
  struct session_work : detail::session_work_queue::work
  {
    Session& session_;
    Request& request_;
    Response& response_;
    Handler handler_;

    session_work(Session& sess, Request& req, Response& res, Handler&& h)
      : session_(sess), request_(req), response_(res), handler_(std::move(h))
    {
    }

    void operator()(session_work_queue&) override
    {
      return net::async_compose<Handler, void(error_code)>(
          process_queue_op<Session, Request, Response>{
              session_, request_, response_},
          handler_,
          session_.get_executor());
    }
  };
  // FIXME: use someone's associated_allocator
  return std::make_unique<session_work>(
      session, request, response, std::move(handler));
}
}

template <typename Endpoint, typename AsyncTransport>
class session
{
  template <typename Session, typename Request, typename Response>
  friend struct detail::process_queue_op;

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

  session(endpoint_type endpoint,
          std::chrono::nanoseconds timeout,
          net::executor& ex)
    : endpoint_(std::move(endpoint)), transport_(timeout, ex)
  {
  }

  template <typename Response, typename Request, typename CompletionToken>
  auto push_request(Request& req, Response& res, CompletionToken&& token)
  {
    // FIXME: move that to the session strand
    net::async_completion<CompletionToken, void(error_code)> comp{token};
    req.keep_alive(true);
    work_queue_.push(
        detail::make_work(*this, req, res, std::move(comp.completion_handler)));
    return comp.result.get();
  }

  auto pending_requests() const
  {
    return work_queue_.size();
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
  template <typename CompletionToken>
  auto async_start(CompletionToken&& token)
  {
    return transport_.async_connect(endpoint_,
                                    std::forward<CompletionToken>(token));
  }

  endpoint_type endpoint_;
  transport_type transport_;
  detail::session_work_queue work_queue_;
  bool first_round_ = true;
};

session(secure_endpoint,
        std::chrono::nanoseconds,
        beast::ssl_stream<beast::tcp_stream> &&)
    ->session<secure_endpoint, ssl_async_transport>;

session(plain_endpoint, std::chrono::nanoseconds, net::executor&)
    ->session<plain_endpoint, tcp_async_transport>;

template <typename AsyncStream>
session(plain_endpoint, std::chrono::nanoseconds, AsyncStream &&)
    ->session<plain_endpoint,
              basic_async_transport<AsyncStream, beast::multi_buffer>>;
}
