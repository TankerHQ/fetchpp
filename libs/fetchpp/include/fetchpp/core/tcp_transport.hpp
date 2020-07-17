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
  AsyncTransport& transport_;
  // FIXME
  std::unique_ptr<tcp::resolver::results_type> results_;
  net::coroutine coro_ = {};

  template <typename Self>
  void operator()(Self& self,
                  error_code ec = {},
                  tcp::resolver::results_type::endpoint_type = {})
  {
    if (ec)
    {
      self.complete(ec);
      return;
    }
    FETCHPP_REENTER(coro_)
    {
      FETCHPP_YIELD beast::get_lowest_layer(transport_)
          .async_connect(*results_, std::move(self));
      self.complete(ec);
    }
  }
};

}

template <typename Executor,
          typename RatePolicy,
          typename DynamicBuffer,
          typename CompletionToken>
auto do_async_connect(
    basic_async_transport<
        beast::basic_stream<net::ip::tcp, Executor, RatePolicy>,
        DynamicBuffer>& ts,
    std::string_view,
    tcp::resolver::results_type resolved_results,
    CompletionToken&& token)
{
  return net::async_compose<CompletionToken, void(error_code)>(
      detail::async_connect_op<basic_async_transport<
          beast::basic_stream<net::ip::tcp, Executor, RatePolicy>,
          DynamicBuffer>>{ts,
                          std::make_unique<tcp::resolver::results_type>(
                              std::move(resolved_results))},
      token,
      ts);
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
  auto& layer = beast::get_lowest_layer(ts);

  auto ec = error_code{};
  layer.socket().cancel(ec);
  beast::close_socket(layer);
  net::post(beast::bind_front_handler(std::move(comp.completion_handler),
                                      error_code{}));
  return comp.result.get();
}

using tcp_async_transport =
    basic_async_transport<beast::tcp_stream, beast::multi_buffer>;
}
