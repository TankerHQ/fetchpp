#pragma once

#include <fetchpp/core/detail/client.hpp>
#include <fetchpp/core/session.hpp>
#include <fetchpp/http/response.hpp>

#include <boost/asio/strand.hpp>

#include <boost/asio/ssl/context.hpp>

#include <fetchpp/alias/error_code.hpp>
#include <fetchpp/alias/net.hpp>
#include <fetchpp/alias/ssl.hpp>

#include <chrono>
#include <deque>
#include <memory>

namespace fetchpp
{
class client
{
public:
  explicit client(net::executor ex,
                  std::chrono::nanoseconds = std::chrono::seconds(30));
  explicit client(net::io_context& ioc,
                  std::chrono::nanoseconds = std::chrono::seconds(30));
  client(net::executor ex, std::chrono::nanoseconds, net::ssl::context context);
  client(net::io_context& ioc,
         std::chrono::nanoseconds,
         net::ssl::context context);

  using executor_type = net::strand<net::executor>;

  executor_type get_executor() const;
  std::size_t max_pending_per_session() const;
  void set_max_pending_per_session(std::size_t pending);
  std::size_t session_count() const;
  void set_verify_peer(bool v);
  net::ssl::context& context();

  template <typename CompletionToken>
  auto async_stop(CompletionToken&& token)
  {
    return net::async_compose<CompletionToken, void(error_code)>(
        detail::client_stop_op{secure_sessions_, plain_sessions_},
        token,
        strand_);
  }

private:
  template <typename AsyncTransport, typename Endpoint>
  auto& get_session(Endpoint endpoint,
                    std::deque<session<Endpoint, AsyncTransport>>& sessions)
  {
    auto found = std::find_if(
        sessions.begin(), sessions.end(), [&](auto const& session) {
          return session.endpoint() == endpoint &&
                 session.pending_requests() < max_pending_per_session();
        });
    if (found == sessions.end())
    {
      if constexpr (Endpoint::is_secure::value)
        return sessions.emplace_back(
            std::move(endpoint), this->timeout_, this->strand_, this->context_);
      else
        return sessions.emplace_back(
            std::move(endpoint), this->timeout_, this->strand_);
    }
    return *found;
  }

public:
  template <typename Endpoint>
  auto& get_session(Endpoint endpoint)
  {
    if constexpr (Endpoint::is_secure::value)
      return this->get_session(std::move(endpoint), this->secure_sessions_);
    else
      return this->get_session(std::move(endpoint), this->plain_sessions_);
  }

  template <typename Request, typename CompletionToken>
  auto async_fetch(Request request, CompletionToken&& token)
  {
    using async_completion_t =
        detail::async_http_completion<CompletionToken, http::response>;
    using handler_t = typename async_completion_t::completion_handler_type;

    async_completion_t async_comp{token};
    auto op =
        detail::client_fetch_op<client, Request, http::response, handler_t>(
            *this,
            std::move(request),
            std::move(async_comp.completion_handler));
    return async_comp.result.get();
  }

private:
  executor_type strand_;
  std::chrono::nanoseconds timeout_;
  std::size_t max_pending_ = 10u;
  net::ssl::context context_;
  std::deque<session<plain_endpoint, tcp_async_transport>> plain_sessions_;
  std::deque<session<secure_endpoint, ssl_async_transport>> secure_sessions_;
};
}
