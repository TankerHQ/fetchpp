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
template <typename Response, typename Request, typename CompletionToken>
auto async_fetch(net::executor ex,
                 net::ssl::context& sslc,
                 Request request,
                 CompletionToken&& token)
    -> detail::async_http_return_type_t<CompletionToken, Response>
{
  static_assert(Request::is_request::value == true,
                "Request type is not valid");
  static_assert(Response::is_request::value == false,
                "Response type is not valid");

  return detail::async_fetch<Response, Request, CompletionToken>(
      ex, sslc, std::move(request), std::move(token));
}

template <typename Response, typename Request, typename CompletionToken>
auto async_fetch(net::ssl::context& sslc,
                 Request request,
                 CompletionToken&& token)
    -> detail::async_http_return_type_t<CompletionToken, Response>
{
  static_assert(Request::is_request::value == true,
                "Request type is not valid");
  static_assert(Response::is_request::value == false,
                "Response type is not valid");
  return async_fetch<Response, Request, CompletionToken>(
      net::system_executor(), sslc, std::move(request), std::move(token));
}

template <typename Response, typename Request, typename CompletionToken>
auto async_fetch(net::executor ex, Request request, CompletionToken&& token)
    -> detail::async_http_return_type_t<CompletionToken, Response>
{
  static_assert(Request::is_request::value == true,
                "Request type is not valid");
  static_assert(Response::is_request::value == false,
                "Response type is not valid");
  return detail::async_fetch<Response, Request, CompletionToken>(
      ex, std::move(request), std::move(token));
}

template <typename Response, typename Request, typename CompletionToken>
auto async_fetch(Request request, CompletionToken&& token)
    -> detail::async_http_return_type_t<CompletionToken, Response>
{
  static_assert(Request::is_request::value == true,
                "Request type is not valid");
  static_assert(Response::is_request::value == false,
                "Response type is not valid");
  return detail::async_fetch<Response, Request, CompletionToken>(
      net::system_executor(), std::move(request), std::move(token));
}

/// Use fetchpp::http::response

template <typename Request, typename CompletionToken>
auto async_fetch(net::executor ex,
                 net::ssl::context& sslc,
                 Request request,
                 CompletionToken&& token)
    -> detail::async_http_return_type_t<CompletionToken, http::response>
{
  static_assert(Request::is_request::value == true,
                "Request type is not valid");
  return async_fetch<http::response>(ex, sslc, std::move(request), token);
}

template <typename Request, typename CompletionToken>
auto async_fetch(net::executor ex, Request request, CompletionToken&& token)
    -> detail::async_http_return_type_t<CompletionToken, http::response>
{
  static_assert(Request::is_request::value == true,
                "Request type is not valid");
  return async_fetch<http::response>(ex, std::move(request), token);
}

}
