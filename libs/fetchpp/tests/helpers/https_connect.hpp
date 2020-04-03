#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/stream.hpp>

#include <fetchpp/alias/net.hpp>
#include <fetchpp/alias/tcp.hpp>

namespace test::helpers
{
inline auto http_resolve_domain(fetchpp::net::io_context& ioc,
                                std::string const& domain)
{
  fetchpp::tcp::resolver resolver(ioc);
  return resolver.resolve(domain, "https");
}

template <typename Stream>
void http_ssl_connect(fetchpp::net::io_context& ioc,
                      Stream& stream,
                      std::string const& domain)
{
  auto results = http_resolve_domain(ioc, domain);
  fetchpp::beast::get_lowest_layer(stream).connect(results);
  stream.handshake(fetchpp::net::ssl::stream_base::client);
}
}
