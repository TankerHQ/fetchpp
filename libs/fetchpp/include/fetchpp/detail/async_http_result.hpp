#pragma once

#include <boost/asio/async_result.hpp>

#include <fetchpp/alias/net.hpp>

namespace fetchpp::detail
{
template <typename CompletionToken, typename Response>
using async_http_result =
    typename net::async_result<typename std::decay_t<CompletionToken>,
                               void(error_code, Response)>;

template <typename CompletionToken, typename Response>
using async_http_result_t =
    typename async_http_result<CompletionToken, Response>::return_type;

}
