#pragma once

#include <fetchpp/core/ssl_transport.hpp>
#include <fetchpp/core/tcp_transport.hpp>

#include <fetchpp/core/detail/async_http_result.hpp>
#include <fetchpp/core/detail/endpoint.hpp>
#include <fetchpp/core/detail/http_stable_async.hpp>

#include <fetchpp/alias/beast.hpp>

#include <deque>

namespace fetchpp::detail
{
template <typename Client, typename Request, typename Response>
struct client_fetch_data
{
  Client& client;
  Request req;
  Response res;

  client_fetch_data(Client& client, Request&& request)
    : client{client}, req{std::move(request)}, res{}
  {
  }
};

template <typename Client,
          typename Request,
          typename Response,
          typename CompletionHandler>
struct client_fetch_op
  : stable_async_t<typename Client::executor_type, CompletionHandler>
{
  using base_type_t =
      stable_async_t<typename Client::executor_type, CompletionHandler>;
  using data_t = client_fetch_data<Client, Request, Response>;

  net::coroutine coro;
  data_t& data;

  client_fetch_op(Client& client, Request&& req, CompletionHandler&& handler)
    : base_type_t(std::move(handler), client.get_executor()),
      coro(),
      data(beast::allocate_stable<data_t>(*this, client, std::move(req)))
  {
    if (auto const& uri = data.req.uri(); http::is_ssl_involved(uri))
      start(to_endpoint<true>(uri));
    else
      start(to_endpoint<false>(uri));
  }

  template <bool isSecure>
  void start(basic_endpoint<isSecure> endpoint)
  {
    auto& session = data.client.get_session(std::move(endpoint));
    session.push_request(data.req, data.res, std::move(*this));
  }

  void operator()(error_code ec = {})
  {
    // pin the response before data_t is destroyed
    auto res = std::move(data.res);
    this->complete(false, ec, std::move(res));
  }
};

template <typename Session>
struct client_stop_sessions_op
{
  std::deque<Session>& sessions_;
  typename std::deque<Session>::iterator begin_ = {};
  typename std::deque<Session>::iterator end_ = {};
  net::coroutine coro_ = {};

  template <typename Self>
  void operator()(Self& self, error_code ec = {})
  {
    FETCHPP_REENTER(coro_)
    {
      begin_ = sessions_.begin();
      end_ = sessions_.end();
      while (begin_ != end_)
      {
        FETCHPP_YIELD begin_->async_stop(std::move(self));
        ++begin_;
      }
      self.complete(ec);
    }
  }
};
template <typename Session>
client_stop_sessions_op(std::deque<Session>&)->client_stop_sessions_op<Session>;

template <typename SessionPool, typename CompletionToken>
auto async_stop_session_pool(SessionPool& sessions, CompletionToken&& token)
{
  return net::async_compose<CompletionToken, void(error_code)>(
      client_stop_sessions_op{sessions}, token);
}

template <typename SecureSessions, typename PlainSessions>
struct client_stop_op
{
  SecureSessions& secure_sessions_;
  PlainSessions& plain_sessions_;
  net::coroutine coro_ = {};

  template <typename Self>
  void operator()(Self& self, error_code ec = {})
  {
    if (ec)
    {
      self.complete(ec);
      return;
    }
    FETCHPP_REENTER(coro_)
    {
      FETCHPP_YIELD async_stop_session_pool(secure_sessions_, std::move(self));
      FETCHPP_YIELD async_stop_session_pool(plain_sessions_, std::move(self));
      self.complete(ec);
    }
  }
};
template <typename Session1, typename Session2>
client_stop_op(std::deque<Session1>&, std::deque<Session2>&)
    ->client_stop_op<std::deque<Session1>, std::deque<Session2>>;
}
