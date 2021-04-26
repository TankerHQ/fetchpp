#pragma once

#include <fetchpp/core/detail/client.hpp>
#include <fetchpp/core/detail/overloaded.hpp>
#include <fetchpp/core/session.hpp>
#include <fetchpp/http/proxy.hpp>
#include <fetchpp/http/response.hpp>

#include <boost/asio/ssl/context.hpp>
#include <boost/asio/strand.hpp>
#include <boost/variant2/variant.hpp>

#include <fetchpp/alias/error_code.hpp>
#include <fetchpp/alias/net.hpp>
#include <fetchpp/alias/ssl.hpp>

#include <chrono>
#include <deque>

namespace fetchpp
{

class client
{
  template <typename Client, typename Handler>
  friend struct detail::client_stop_sessions_op;

public:
  explicit client(net::executor ex,
                  std::chrono::nanoseconds = std::chrono::seconds(30));
  explicit client(net::io_context& ioc,
                  std::chrono::nanoseconds = std::chrono::seconds(30));
  client(net::executor ex, std::chrono::nanoseconds, net::ssl::context context);
  client(net::io_context& ioc,
         std::chrono::nanoseconds,
         net::ssl::context context);

  using plain_session = session<plain_endpoint, tcp_async_transport>;
  using secure_session = session<secure_endpoint, ssl_async_transport>;
  using tunnel_session = session<tunnel_endpoint, tunnel_async_transport>;
  using session_adapter =
      boost::variant2::variant<plain_session, secure_session, tunnel_session>;
  using sessions = std::deque<session_adapter>;

  using internal_executor_type = net::strand<net::executor>;
  using default_executor_type =
      typename internal_executor_type::inner_executor_type;

  auto get_internal_executor() const -> internal_executor_type;
  auto get_default_executor() const -> default_executor_type;

  std::size_t max_pending_per_session() const;
  void set_max_pending_per_session(std::size_t pending);
  std::size_t session_count() const;
  void set_verify_peer(bool v);
  net::ssl::context& context();
  void add_proxy(http::proxy_match, http::proxy);
  void set_proxies(http::proxy_map);
  http::proxy_map const& proxies() const;

  template <typename CompletionToken>
  auto async_stop(GracefulShutdown graceful, CompletionToken&& token)
  {
    auto launch = [](auto&& handler, client* cl, GracefulShutdown gr) {
      auto op = detail::client_stop_sessions_op{gr, *cl, std::move(handler)};
      net::dispatch(std::move(op));
    };
    return net::async_initiate<CompletionToken, void(error_code)>(
        std::move(launch), token, this, graceful);
  }

  template <typename CompletionToken>
  auto async_stop(CompletionToken&& token)
  {
    return async_stop(GracefulShutdown::Yes,
                      std::forward<CompletionToken>(token));
  }

  template <
      typename Endpoint,
      typename Session = typename detail::session_for_tunnel<Endpoint>::type>
  auto& get_session(Endpoint endpoint)
  {
    auto found = std::find_if(
        sessions_.begin(), sessions_.end(), [&](auto const& adapter) {
          return boost::variant2::visit(
              detail::overloaded{[&](Session const& session) {
                                   return endpoint == session.endpoint() &&
                                          session.pending_requests() <
                                              max_pending_per_session();
                                 },
                                 [&](auto const&) { return false; }},
              adapter);
        });
    if (found == sessions_.end())
    {
      if constexpr (Session::endpoint_type::is_secure::value)
        return sessions_.emplace_back(boost::variant2::in_place_type<Session>,
                                      std::move(endpoint),
                                      this->strand_,
                                      this->timeout_,
                                      this->context_);
      else
        return sessions_.emplace_back(boost::variant2::in_place_type<Session>,
                                      std::move(endpoint),
                                      this->strand_,
                                      this->timeout_);
    }
    return *found;
  }

  template <typename Request, typename CompletionToken>
  auto async_fetch(Request request, CompletionToken&& token)
  {
    auto launch = [](auto&& handler, client* cl, Request request) {
      auto op = detail::client_fetch_op(
          *cl, std::move(request), std::forward<decltype(handler)>(handler));
      net::dispatch(std::move(op));
    };
    return net::async_initiate<CompletionToken,
                               void(error_code, http::response)>(
        std::move(launch), token, this, std::move(request));
  }

private:
  internal_executor_type strand_;
  std::chrono::nanoseconds timeout_;
  std::size_t max_pending_ = 10u;
  net::ssl::context context_;
  sessions sessions_;
  http::proxy_map proxies_;
};
}
