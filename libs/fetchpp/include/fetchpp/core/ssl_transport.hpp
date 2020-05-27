#pragma once

#include <fetchpp/core/basic_transport.hpp>
#include <fetchpp/core/detail/coroutine.hpp>
#include <fetchpp/core/endpoint.hpp>

#include <boost/asio/compose.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/core/multi_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>

#include <fetchpp/alias/beast.hpp>
#include <fetchpp/alias/error_code.hpp>
#include <fetchpp/alias/tcp.hpp>

namespace fetchpp
{
namespace detail
{
template <typename AsyncTransport>
struct async_ssl_connect_op
{
  base_endpoint& endpoint;
  AsyncTransport& transport;
  tcp::resolver resolver;
  net::coroutine coro_;

  async_ssl_connect_op(base_endpoint& endpoint, AsyncTransport& transport)
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
      // https://www.cloudflare.com/learning/ssl/what-is-sni/
      if (!SSL_set_tlsext_host_name(transport.next_layer().native_handle(),
                                    endpoint.domain().data()))
        return self.complete(error_code{static_cast<int>(::ERR_get_error()),
                                        net::error::get_ssl_category()});

      FETCHPP_YIELD beast::get_lowest_layer(transport.next_layer())
          .async_connect(results, std::move(self));
      self.complete(ec);
    }
  }

  template <typename Self>
  void operator()(Self& self,
                  error_code ec,
                  tcp::resolver::results_type::endpoint_type)
  {
    if (ec)
    {
      self.complete(ec);
      return;
    }
    transport.next_layer().async_handshake(net::ssl::stream_base::client,
                                           std::move(self));
  }
};

template <typename AsyncStream>
struct async_ssl_close_op
{
  AsyncStream& stream;
  net::coroutine coro_ = {};

  template <typename Self>
  void operator()(Self& self, error_code ec = {})
  {
    FETCHPP_REENTER(coro_)
    {
      beast::get_lowest_layer(stream).socket().cancel();
      FETCHPP_YIELD stream.async_shutdown(std::move(self));
      if (ec == net::error::eof)
        // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
        ec = {};
      else if (ec == net::ssl::error::stream_truncated)
        // lots of implementation do not respect the graceful shutdown
        ec = {};
      if (ec)
      {
        self.complete(ec);
        return;
      }
      beast::close_socket(beast::get_lowest_layer(stream));
      self.complete(ec);
    }
  }
};
template <typename AsyncStream>
async_ssl_close_op(AsyncStream&)->async_ssl_close_op<AsyncStream>;

}

template <typename NextLayer, typename DynamicBuffer, typename CompletionToken>
auto do_async_connect(
    detail::base_endpoint& endpoint,
    basic_async_transport<beast::ssl_stream<NextLayer>, DynamicBuffer>& ts,
    CompletionToken&& token)
{
  return net::async_compose<CompletionToken, void(error_code)>(
      detail::async_ssl_connect_op{endpoint, ts}, token, ts);
}

template <typename NextLayer, typename DynamicBuffer, typename CompletionToken>
auto async_close(
    basic_async_transport<beast::ssl_stream<NextLayer>, DynamicBuffer>& ts,
    CompletionToken&& token)
{
  return net::async_compose<CompletionToken, void(error_code)>(
      detail::async_ssl_close_op{ts.next_layer()}, token, ts);
}

using ssl_async_transport =
    basic_async_transport<beast::ssl_stream<beast::tcp_stream>,
                          beast::multi_buffer>;

}
