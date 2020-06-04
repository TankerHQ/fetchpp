#pragma once

#include <fetchpp/core/session.hpp>
#include <fetchpp/core/ssl_transport.hpp>
#include <fetchpp/core/tcp_transport.hpp>
#include <fetchpp/http/request.hpp>
#include <fetchpp/http/response.hpp>
#include <fetchpp/http/url.hpp>

#include <boost/asio/strand.hpp>

#include <boost/asio/ssl/context.hpp>

#include <fetchpp/core/detail/async_http_result.hpp>
#include <fetchpp/core/detail/http_stable_async.hpp>

#include <fetchpp/alias/beast.hpp>
#include <fetchpp/alias/error_code.hpp>
#include <fetchpp/alias/net.hpp>
#include <fetchpp/alias/ssl.hpp>

#include <deque>
#include <memory>
#include <queue>

namespace fetchpp
{
namespace detail
{

template <typename Client, typename Request, typename Response>
struct client_fetch_data
{
  Client& client;
  Request req;
  Response res;

  client_fetch_data(Client& client, Request request)
    : client{client}, req{request}, res{}
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
  }

  template <bool is_ssl_involved>
  void start()
  {
    auto& session =
        data.client.template get_session<is_ssl_involved>(data.req.uri());
    session.push_request(data.req, data.res, std::move(*this));
  }

  void operator()(error_code ec = {})
  {
    this->complete(false, ec, std::move(std::exchange(data.res, Response{})));
  }
};

}

class client
{
public:
  client(net::executor ex);
  client(net::io_context& ioc);
  client(net::executor ex, net::ssl::context context);
  client(net::io_context& ioc, net::ssl::context context);

  using executor_type = net::strand<net::executor>;

  executor_type get_executor() const;

  auto max_pending_per_session() const
  {
    return 2u;
  }

private:
  template <typename AsyncTransport>
  auto& get_session(std::string const& domain,
                    std::uint16_t port,
                    std::deque<session<AsyncTransport>>& sessions)
  {
    auto found = std::find_if(
        sessions.begin(), sessions.end(), [&](auto const& session) {
          return session.domain() == domain && session.port() == port &&
                 session.pending_requests() < max_pending_per_session();
        });
    if (found == sessions.end())
    {
      if constexpr (std::is_same_v<AsyncTransport, ssl_async_transport>)
        return sessions.emplace_back(
            AsyncTransport(domain, port, this->strand_, this->context_));
      else
        return sessions.emplace_back(
            AsyncTransport(domain, port, this->strand_));
    }
    return *found;
  }

public:
  template <bool is_ssl_involved>
  auto& get_session(http::url const& url)
  {
    if constexpr (is_ssl_involved)
      return this->get_session(url.domain(), url.port(), this->ssl_sessions_);
    else
      return this->get_session(url.domain(), url.port(), this->tcp_sessions_);
  }

  template <typename Request, typename CompletionToken>
  auto async_fetch(Request request, CompletionToken&& token)
  {
    using async_completion_t =
        detail::async_http_completion<CompletionToken, http::response>;
    using handler_t = typename async_completion_t::completion_handler_type;

    async_completion_t async_comp{token};
    bool is_ssl = request.uri().is_ssl_involved();
    auto op =
        detail::client_fetch_op<client, Request, http::response, handler_t>(
            *this,
            std::move(request),
            std::move(async_comp.completion_handler));
    if (is_ssl)
      op.template start<true>();
    else
      op.template start<false>();
    return async_comp.result.get();
  }

  auto session_count() const
  {
    return tcp_sessions_.size() + ssl_sessions_.size();
  }

  executor_type strand_;
  net::ssl::context context_;
  std::deque<session<tcp_async_transport>> tcp_sessions_;
  std::deque<session<ssl_async_transport>> ssl_sessions_;
};
}
