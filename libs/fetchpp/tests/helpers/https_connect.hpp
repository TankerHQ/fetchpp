#pragma once

#include <boost/asio/compose.hpp>
#include <boost/asio/coroutine.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core/error.hpp>
#include <skyr/url.hpp>

#include <boost/beast/core/stream_traits.hpp>

namespace test::helpers
{
namespace net = boost::asio;
namespace bb = boost::beast;

net::ip::tcp::resolver::results_type http_resolve_domain(net::any_io_executor ex,
                                                         skyr::url const& url);

template <typename Stream>
void http_connect(Stream& stream, skyr::url const& url)
{
  auto results = http_resolve_domain(stream.get_executor(), url);
  bb::get_lowest_layer(stream).connect(results);
}

template <typename Stream>
void https_connect(Stream& stream, skyr::url const& url)
{
  http_connect(stream, url);
  stream.handshake(net::ssl::stream_base::client);
}

template <typename AsyncStream, typename CompletionToken>
auto async_ssl_connect(AsyncStream& stream,
                       net::ip::tcp::endpoint endpoint,
                       CompletionToken&& token)
{
  return net::async_compose<CompletionToken, void(bb::error_code)>(
      [&, endpoint, coro_ = net::coroutine{}](auto& self,
                                              bb::error_code ec = {}) mutable {
        if (ec)
        {
          self.complete(ec);
          return;
        }
        BOOST_ASIO_CORO_REENTER(coro_)
        {
          BOOST_ASIO_CORO_YIELD bb::get_lowest_layer(stream).async_connect(
              endpoint, std::move(self));
          BOOST_ASIO_CORO_YIELD stream.async_handshake(
              net::ssl::stream_base::handshake_type::client, std::move(self));
          self.complete(ec);
        }
      },
      token,
      stream);
}
}
