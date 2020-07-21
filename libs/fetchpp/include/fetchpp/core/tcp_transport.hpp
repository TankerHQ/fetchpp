#pragma once

#include <fetchpp/core/basic_transport.hpp>
#include <fetchpp/core/endpoint.hpp>

#include <boost/asio/compose.hpp>
#include <boost/beast/core/tcp_stream.hpp>

#include <fetchpp/alias/beast.hpp>
#include <fetchpp/alias/tcp.hpp>

namespace fetchpp
{
namespace detail
{
template <typename AsyncTransport>
struct async_connect_op
{
  base_endpoint& endpoint;
  AsyncTransport& transport;
  tcp::resolver resolver;
  net::coroutine coro_;

  async_connect_op(base_endpoint& endpoint, AsyncTransport& transport)
    : endpoint(endpoint),
      transport(transport),
      resolver(transport.get_executor())
  {
  }

  template <typename Self>
  void operator()(Self& self,
                  error_code ec = {},
                  tcp::resolver::results_type results = {})
  {
    if (ec)
    {
      self.complete(ec);
      return;
    }
    FETCHPP_REENTER(coro_)
    {
      FETCHPP_YIELD resolver.async_resolve(
          endpoint.domain(),
          std::to_string(endpoint.port()),
          net::ip::resolver_base::numeric_service,
          std::move(self));
      FETCHPP_YIELD beast::get_lowest_layer(transport).async_connect(
          results, std::move(self));
    }
  }

  template <typename Self>
  void operator()(Self& self,
                  error_code ec,
                  tcp::resolver::results_type::endpoint_type)
  {
    self.complete(ec);
  }
};
}

template <typename Executor,
          typename RatePolicy,
          typename DynamicBuffer,
          typename CompletionToken>
auto do_async_connect(
    detail::base_endpoint& endpoint,
    basic_async_transport<
        beast::basic_stream<net::ip::tcp, Executor, RatePolicy>,
        DynamicBuffer>& ts,
    CompletionToken&& token)
{
  return net::async_compose<CompletionToken, void(error_code)>(
      detail::async_connect_op{endpoint, ts}, token, ts);
}

template <typename Executor,
          typename RatePolicy,
          typename DynamicBuffer,
          typename CompletionToken>
auto async_close(basic_async_transport<
                     beast::basic_stream<net::ip::tcp, Executor, RatePolicy>,
                     DynamicBuffer>& ts,
                 CompletionToken&& token)
{
  net::async_completion<CompletionToken, void(error_code)> comp{token};
  beast::close_socket(beast::get_lowest_layer(ts));
  net::post(beast::bind_front_handler(std::move(comp.completion_handler),
                                      error_code{}));
  return comp.result.get();
}

using tcp_async_transport =
    basic_async_transport<beast::tcp_stream, beast::multi_buffer>;
}
