#pragma once

#include <fetchpp/core/ssl_transport.hpp>
#include <fetchpp/core/tcp_transport.hpp>

#include <fetchpp/core/detail/async_http_result.hpp>
#include <fetchpp/core/detail/endpoint.hpp>
#include <fetchpp/core/detail/http_stable_async.hpp>
#include <fetchpp/core/endpoint.hpp>
#include <fetchpp/core/session.hpp>
#include <fetchpp/http/proxy.hpp>
#include <fetchpp/http/response.hpp>
#include <fetchpp/net/make_dispatch.hpp>
#include <fetchpp/net/make_post.hpp>

#include <fetchpp/alias/beast.hpp>

#include <boost/variant2/variant.hpp>

#include <deque>

namespace fetchpp::detail
{
template <typename Tunnel>
struct session_for_tunnel;

template <>
struct session_for_tunnel<tunnel_endpoint>
{
  using type = session<tunnel_endpoint, tunnel_async_transport>;
};
template <>
struct session_for_tunnel<secure_endpoint>
{
  using type = session<secure_endpoint, ssl_async_transport>;
};
template <>
struct session_for_tunnel<plain_endpoint>
{
  using type = session<plain_endpoint, tcp_async_transport>;
};

template <typename Client, typename Request, typename Handler>
struct client_fetch_data
{
  Client& client;
  Request req;
  Handler handler;
  http::response res = {};

  client_fetch_data(Client& client, Request&& request, Handler&& h)
    : client(client), req(std::move(request)), handler(std::move(h))
  {
  }
};

template <typename Client, typename Request, typename Handler>
struct client_fetch_op
{
  using data_t = client_fetch_data<Client, Request, Handler>;
  std::unique_ptr<data_t> data;

  client_fetch_op(Client& client, Request&& req, Handler&& handler)
    : data(std::make_unique<data_t>(
          client, std::forward<Request>(req), std::forward<Handler>(handler)))
  {
  }

  using executor_type = typename Client::internal_executor_type;
  auto get_executor() const
  {
    return data->client.get_internal_executor();
  }

  void operator()()
  {
    BOOST_ASSERT(data->client.get_internal_executor().running_in_this_thread());

    auto const& proxy =
        http::select_proxy(data->client.proxies(), data->req.uri());
    auto request_pusher = [&](auto& s) {
      return s.push_request(data->req, data->res, std::move(*this));
    };

    if (proxy.has_value())
    {
      auto& session = data->client.get_session(
          tunnel_endpoint{to_endpoint<false>(proxy->url()),
                          to_endpoint<true>(data->req.uri())});
      boost::variant2::visit(request_pusher, session);
    }
    else if (auto const& uri = data->req.uri(); http::is_ssl_involved(uri))
    {
      auto& session = data->client.get_session(to_endpoint<true>(uri));
      boost::variant2::visit(request_pusher, session);
    }
    else
    {
      auto& session = data->client.get_session(to_endpoint<false>(uri));
      boost::variant2::visit(request_pusher, session);
    }
  }

  void operator()(error_code ec)
  {
    net::make_post(beast::bind_front_handler(
                       std::move(data->handler), ec, std::move(data->res)),
                   data->client.get_internal_executor());
  }
};
template <typename Client, typename Request, typename CompletionHandler>
client_fetch_op(Client&, Request, CompletionHandler)
    -> client_fetch_op<Client, Request, CompletionHandler>;

template <typename Client, typename Handler>
struct client_stop_sessions_op
{
  GracefulShutdown graceful_;
  Client& client_;
  Handler handler_;
  error_code ec_ = {};

  typename Client::sessions::iterator begin_ = {};
  typename Client::sessions::iterator end_ = {};
  net::coroutine coro_ = {};

  void operator()(error_code ec = {})
  {
    FETCHPP_REENTER(coro_)
    {
      begin_ = client_.sessions_.begin();
      end_ = client_.sessions_.end();
      while (begin_ != end_)
      {
        FETCHPP_YIELD boost::variant2::visit(
            [&](auto& session) {
              return session.async_stop(graceful_, std::move(*this));
            },
            *begin_);
        if (ec)
          ec_ = ec;
        ++begin_;
      }
      if (ec_)
        ec = net::error::operation_aborted;
      net::make_post(beast::bind_front_handler(std::move(handler_), ec),
                     get_executor().get_inner_executor());
    }
  }
  using executor_type = typename Client::internal_executor_type;
  auto get_executor() const
  {
    return client_.get_internal_executor();
  }
};
template <typename Client, typename Handler>
client_stop_sessions_op(GracefulShutdown, Client&, Handler&&)
    -> client_stop_sessions_op<Client, Handler>;

}
