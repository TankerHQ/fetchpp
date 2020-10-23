#pragma once

#include <fetchpp/http/request.hpp>
#include <fetchpp/http/response.hpp>

#include <fetchpp/core/detail/async_http_result.hpp>
#include <fetchpp/core/detail/fetch.hpp>

#include <fetchpp/alias/beast.hpp>
#include <fetchpp/alias/error_code.hpp>
#include <fetchpp/alias/net.hpp>

namespace fetchpp
{
template <typename CompletionToken>
auto async_fetch(net::executor ex,
                 net::ssl::context& sslc,
                 http::request request,
                 CompletionToken&& token)
    -> detail::async_http_return_type_t<CompletionToken, http::response>
{
  return detail::async_fetch<http::response>(
      ex, sslc, std::move(request), std::forward<CompletionToken>(token));
}

template <typename CompletionToken>
auto async_fetch(net::executor ex,
                 http::request request,
                 CompletionToken&& token)
    -> detail::async_http_return_type_t<CompletionToken, http::response>
{
  return detail::async_fetch<http::response>(
      ex, std::move(request), std::forward<CompletionToken>(token));
}
}
