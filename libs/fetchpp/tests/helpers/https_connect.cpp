#include "https_connect.hpp"

namespace test::helpers
{
namespace
{
// copied from fetchpp url.hpp
std::int16_t safe_port(skyr::url const& uri)
{
  if (uri.port().empty())
  {
    auto const proto = uri.protocol();
    auto const scheme = std::string_view{proto.data(), proto.size() - 1};
    if (auto port = skyr::url::default_port(scheme); port.has_value())
      return *port;
  }
  else if (auto port = uri.port<std::int16_t>(); port.has_value())
  {
    return *port;
  }
  throw std::domain_error("cannot deduce port");
}
}

net::ip::tcp::resolver::results_type http_resolve_domain(net::executor ex,
                                                         skyr::url const& url)
{
  net::ip::tcp::resolver resolver(ex);
  auto port = safe_port(url);
  return resolver.resolve(url.hostname(),
                          std::to_string(port),
                          net::ip::resolver_base::numeric_service);
}
}
