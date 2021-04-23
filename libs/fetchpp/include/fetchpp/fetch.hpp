#pragma once

#include <fetchpp/http/request.hpp>
#include <fetchpp/http/response.hpp>

#include <fetchpp/core/detail/fetch.hpp>
#include <fetchpp/core/ssl_transport.hpp>

#include <boost/asio/ssl/context.hpp>

#include <fetchpp/alias/net.hpp>
#include <fetchpp/alias/ssl.hpp>

#include <functional>

namespace fetchpp
{
template <typename Response = http::response,
          typename Request,
          typename CompletionToken>
auto async_fetch(net::executor ex,
                 net::ssl::context& sslc,
                 Request request,
                 CompletionToken&& token)
{
  auto endpoint = detail::to_endpoint<true>(request.uri());
  auto launch = [](auto&& handler,
                   net::executor ex,
                   auto endpoint,
                   net::ssl::context& sslc,
                   Request request) {
    auto s = detail::make_state<detail::base_state, Response>(
        std::move(endpoint),
        ssl_async_transport(ex, std::chrono::seconds(30), sslc),
        std::move(request),
        std::forward<decltype(handler)>(handler));
    net::dispatch(detail::simple_fetch_op{std::move(s)});
  };
  return net::async_initiate<CompletionToken, void(error_code, Response)>(
      std::move(launch),
      token,
      ex,
      std::move(endpoint),
      std::ref(sslc),
      std::move(request));
}

template <typename Response = http::response,
          typename Request,
          typename CompletionToken>
auto async_fetch(net::executor ex, Request request, CompletionToken&& token)
{
  if (http::is_ssl_involved(request.uri()))
  {
    return net::async_compose<CompletionToken, void(error_code, Response)>(
        [ex,
         req = std::move(request),
         sslc = net::ssl::context(net::ssl::context::tls_client),
         coro_ = net::coroutine{}](
            auto& self, error_code ec = {}, Response res = {}) mutable {
          FETCHPP_REENTER(coro_)
          {
            FETCHPP_YIELD async_fetch(ex, sslc, req, std::move(self));
            self.complete(ec, res);
          }
        },
        token,
        ex);
  }
  else
  {
    return detail::async_fetch<Response>(
        ex, std::move(request), std::forward<CompletionToken>(token));
  }
}
}
