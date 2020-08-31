#pragma once

#include <fetchpp/core/detail/client.hpp>
#include <fetchpp/core/session.hpp>
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
  using session_adapter =
      boost::variant2::variant<plain_session, secure_session>;

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
        [this, coro_ = net::coroutine{}](auto& self,
                                         error_code ec = {}) mutable {
          FETCHPP_REENTER(coro_)
          {
            FETCHPP_YIELD detail::async_stop_session_pool(this->sessions_,
                                                          std::move(self));
            self.complete(ec);
          }
        },
        token,
        strand_);
  }

  template <typename Endpoint>
  auto& get_session(Endpoint endpoint)
  {
    auto found = std::find_if(
        sessions_.begin(), sessions_.end(), [&](auto const& adapter) {
          return boost::variant2::visit(
              [&](auto const& s) {
                return endpoint == s.endpoint() &&
                       s.pending_requests() < max_pending_per_session();
              },
              adapter);
        });
    if (found == sessions_.end())
    {
      if constexpr (Endpoint::is_secure::value)
        return sessions_.emplace_back(
            boost::variant2::in_place_type<secure_session>,
            std::move(endpoint),
            this->timeout_,
            this->strand_,
            this->context_);
      else
        return sessions_.emplace_back(
            boost::variant2::in_place_type<plain_session>,
            std::move(endpoint),
            this->timeout_,
            this->strand_);
    }
    return *found;
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
  std::deque<session_adapter> sessions_;
};
}
