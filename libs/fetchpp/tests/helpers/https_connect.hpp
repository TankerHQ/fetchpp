#pragma once

#include <fetchpp/http/url.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/stream.hpp>

#include <fetchpp/alias/net.hpp>
#include <fetchpp/alias/tcp.hpp>

namespace test::helpers
{
inline auto http_resolve_domain(fetchpp::net::io_context& ioc,
                                fetchpp::http::url const& url)
{
  fetchpp::tcp::resolver resolver(ioc);
  auto port = fetchpp::http::safe_port(url);
  return resolver.resolve(url.hostname(),
                          std::to_string(port),
                          boost::asio::ip::resolver_base::numeric_service);
}

template <typename Stream>
void http_connect(fetchpp::net::io_context& ioc,
                  Stream& stream,
                  fetchpp::http::url const& url)
{
  auto results = http_resolve_domain(ioc, url);
  fetchpp::beast::get_lowest_layer(stream).connect(results);
}

template <typename Stream>
void https_connect(fetchpp::net::io_context& ioc,
                   Stream& stream,
                   fetchpp::http::url const& url)
{
  http_connect(ioc, stream, url);
  stream.handshake(fetchpp::net::ssl::stream_base::client);
}

}
