#pragma once

#include <fetchpp/core/detail/async_http_result.hpp>

#include <boost/beast/core/async_base.hpp>

#include <fetchpp/alias/beast.hpp>

namespace fetchpp::detail
{
template <typename AsyncStream, typename Response, typename CompletionToken>
using stable_async_t = beast::stable_async_base<
    detail::async_http_completion_handler_t<CompletionToken, Response>,
    typename AsyncStream::executor_type>;

}
