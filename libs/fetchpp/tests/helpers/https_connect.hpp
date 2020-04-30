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
  return resolver.resolve(url.domain(),
                          std::to_string(url.port()),
                          boost::asio::ip::resolver_base::numeric_service);
}

template <typename Stream>
void http_ssl_connect(fetchpp::net::io_context& ioc,
                      Stream& stream,
                      fetchpp::http::url const& url)
{
  auto results = http_resolve_domain(ioc, url);
  fetchpp::beast::get_lowest_layer(stream).connect(results);
  stream.handshake(fetchpp::net::ssl::stream_base::client);
}
}
