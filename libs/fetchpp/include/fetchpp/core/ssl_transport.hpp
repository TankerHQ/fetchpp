#pragma once

#include <fetchpp/core/basic_transport.hpp>
#include <fetchpp/core/detail/close_ssl.hpp>
#include <fetchpp/core/detail/coroutine.hpp>

#include <boost/asio/compose.hpp>
#include <boost/beast/core/bind_handler.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/core/multi_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>

#include <fetchpp/alias/beast.hpp>
#include <fetchpp/alias/error_code.hpp>
#include <fetchpp/alias/strings.hpp>
#include <fetchpp/alias/tcp.hpp>

#include <cassert>

namespace fetchpp
{
namespace detail
{
template <typename AsyncTransport>
struct async_ssl_connect_op
{
  AsyncTransport& transport_;
  std::string domain_;
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
      // https://www.cloudflare.com/learning/ssl/what-is-sni/
      if (!SSL_set_tlsext_host_name(transport_.next_layer().native_handle(),
                                    domain_.data()))
        return self.complete(error_code{static_cast<int>(::ERR_get_error()),
                                        net::error::get_ssl_category()});

      FETCHPP_YIELD beast::get_lowest_layer(transport_.next_layer())
          .async_connect(*results_, std::move(self));
      beast::get_lowest_layer(transport_)
          .socket()
          .set_option(net::ip::tcp::no_delay{true}, ec);
      if (ec)
      {
        self.complete(ec);
        return;
      }
      FETCHPP_YIELD transport_.next_layer().async_handshake(
          net::ssl::stream_base::client, std::move(self));
      self.complete(ec);
    }
  }
};
template <typename AsyncTransport>
async_ssl_connect_op(AsyncTransport&,
                     std::string,
                     std::unique_ptr<tcp::resolver::results_type>)
    -> async_ssl_connect_op<AsyncTransport>;

}

template <typename NextLayer, typename DynamicBuffer, typename CompletionToken>
auto do_async_connect(
    basic_async_transport<beast::ssl_stream<NextLayer>, DynamicBuffer>& ts,
    string_view domain,
    tcp::resolver::results_type resolved_results,
    CompletionToken&& token)
{
  return net::async_compose<CompletionToken, void(error_code)>(
      detail::async_ssl_connect_op{
          ts,
          std::string{domain},
          std::make_unique<tcp::resolver::results_type>(resolved_results)},
      token,
      ts);
}

template <typename NextLayer, typename DynamicBuffer, typename CompletionToken>
auto do_async_close(
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
