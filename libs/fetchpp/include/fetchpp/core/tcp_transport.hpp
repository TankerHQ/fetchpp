#pragma once

#include <fetchpp/core/basic_transport.hpp>
#include <fetchpp/core/endpoint.hpp>

#include <boost/asio/compose.hpp>
#include <boost/beast/core/bind_handler.hpp>
#include <boost/beast/core/multi_buffer.hpp>
#include <boost/beast/core/stream_traits.hpp>
#include <boost/beast/core/tcp_stream.hpp>

#include <fetchpp/alias/beast.hpp>
#include <fetchpp/alias/tcp.hpp>

namespace fetchpp
{
namespace detail
{
template <typename AsyncTransport>
struct async_tcp_connect_op
{
  AsyncTransport& transport_;
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
      beast::get_lowest_layer(transport_)
          .socket()
          .set_option(net::ip::tcp::no_delay{true});
      self.complete(ec);
    }
  }
};
template <typename AsyncTransport>
async_tcp_connect_op(AsyncTransport&,
                     std::unique_ptr<tcp::resolver::results_type>)
    -> async_tcp_connect_op<AsyncTransport>;

template <typename AsyncStream>
struct async_tcp_close_op
{
  AsyncStream& stream_;
  net::coroutine coro_ = {};

  template <typename Self>
  void operator()(Self& self, error_code ec = {})
  {
    FETCHPP_REENTER(coro_)
    {
      beast::get_lowest_layer(stream_).socket().cancel(ec);
      assert(!ec);
      // ignore the error
      ec = {};
      // give a chance to canceled handler to finish
      FETCHPP_YIELD net::post(beast::bind_front_handler(std::move(self)));
      beast::close_socket(beast::get_lowest_layer(stream_));
      self.complete(ec);
    }
  }
};
template <typename AsyncStream>
async_tcp_close_op(AsyncStream&) -> async_tcp_close_op<AsyncStream>;
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
      detail::async_tcp_connect_op{
          ts,
          std::make_unique<tcp::resolver::results_type>(
              std::move(resolved_results))},
      token,
      ts);
}

template <typename Executor,
          typename RatePolicy,
          typename DynamicBuffer,
          typename CompletionToken>
auto do_async_close(basic_async_transport<
                        beast::basic_stream<net::ip::tcp, Executor, RatePolicy>,
                        DynamicBuffer>& ts,
                    CompletionToken&& token)
{
  return net::async_compose<CompletionToken, void(error_code)>(
      detail::async_tcp_close_op{ts}, token, ts);
}

using tcp_async_transport =
    basic_async_transport<beast::tcp_stream, beast::multi_buffer>;
}
