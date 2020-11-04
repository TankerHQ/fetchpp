#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <skyr/url.hpp>

#include <boost/beast/core/stream_traits.hpp>

namespace test::helpers
{
namespace net = boost::asio;
namespace bb = boost::beast;

net::ip::tcp::resolver::results_type http_resolve_domain(net::executor ex,
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

}
