#pragma once

#include <boost/asio/is_executor.hpp>
#include <boost/beast/core/async_base.hpp>
#include <boost/beast/core/stream_traits.hpp>

#include <fetchpp/alias/beast.hpp>
#include <fetchpp/alias/net.hpp>

#include <type_traits>

namespace fetchpp::detail
{
template <typename Executor, typename CompletionHandler, typename Enable = void>
struct stable_async_t;

template <typename Executor, typename CompletionHandler>
struct stable_async_t<Executor,
                      CompletionHandler,
                      std::enable_if_t<net::is_executor<Executor>::value>>
  : beast::stable_async_base<CompletionHandler, Executor>
{
  using beast::stable_async_base<CompletionHandler,
                                 Executor>::stable_async_base;
};

template <typename AsyncStream, typename CompletionHandler>
struct stable_async_t<
    AsyncStream,
    CompletionHandler,
    std::enable_if_t<beast::is_async_stream<AsyncStream>::value>>
  : beast::stable_async_base<CompletionHandler,
                             typename AsyncStream::executor_type>
{
  using beast::stable_async_base<
      CompletionHandler,
      typename AsyncStream::executor_type>::stable_async_base;
};

}
