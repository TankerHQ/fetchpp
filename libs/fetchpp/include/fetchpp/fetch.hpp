#pragma once

#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>

#include <fetchpp/detail/async_http_result.hpp>
#include <fetchpp/detail/fetch.hpp>

#include <fetchpp/alias/beast.hpp>
#include <fetchpp/alias/error_code.hpp>
#include <fetchpp/alias/net.hpp>

namespace fetchpp
{
template <typename Response, typename Request, typename CompletionToken>
auto async_fetch(net::io_context& ioc,
                 net::ssl::context& sslc,
                 Request request,
                 CompletionToken&& token)
    -> detail::async_http_return_type_t<CompletionToken, Response>
{
  static_assert(Request::is_request::value == true,
                "Request type is not valid");
  static_assert(Response::is_request::value == false,
                "Response type is not valid");

  return detail::async_fetch_impl<Response, Request, CompletionToken>(
      ioc, sslc, std::move(request), token);
}

template <typename Response, typename Request, typename CompletionToken>
auto async_fetch(net::io_context& ioc, Request request, CompletionToken&& token)
    -> detail::async_http_return_type_t<CompletionToken, Response>
{
  static_assert(Request::is_request::value == true,
                "Request type is not valid");
  static_assert(Response::is_request::value == false,
                "Response type is not valid");
  return net::async_compose<CompletionToken, void(error_code, Response)>(
      detail::fetch_composer_with_ssl<Request, Response>{ioc,
                                                         std::move(request)},
      token,
      ioc);
}

}
