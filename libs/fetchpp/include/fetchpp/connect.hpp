#pragma once

#include <fetchpp/detail/connect_composer.hpp>

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/error.hpp>

#include <fetchpp/alias/error_code.hpp>
#include <fetchpp/alias/net.hpp>
#include <fetchpp/alias/tcp.hpp>

namespace fetchpp
{
template <typename CompletionToken, typename AsyncStream>
auto async_connect(AsyncStream& stream,
                   std::string_view domain,
                   tcp::resolver::results_type& results,
                   CompletionToken&& token) ->
    typename net::async_result<typename std::decay_t<CompletionToken>,
                               void(error_code)>::return_type
{
  return net::async_compose<CompletionToken, void(error_code)>(
      detail::connect_composer<AsyncStream>{stream, domain, results},
      token,
      stream);
}
}
