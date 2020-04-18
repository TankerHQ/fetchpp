#pragma once

#include <boost/asio/async_result.hpp>

#include <fetchpp/alias/net.hpp>

namespace fetchpp::detail
{
template <typename CompletionToken, typename Response>
using async_http_completion =
    typename net::async_completion<CompletionToken, void(error_code, Response)>;

template <typename CompletionToken, typename Response>
using async_http_result_t =
    decltype(async_http_completion<CompletionToken, Response>::result);

template <typename CompletionToken, typename Response>
using async_http_completion_handler_t =
    typename async_http_completion<CompletionToken,
                                   Response>::completion_handler_type;

template <typename CompletionToken, typename Response>
using async_http_return_type_t =
    typename async_http_result_t<CompletionToken, Response>::return_type;

}
