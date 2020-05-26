#include <fetchpp/core/basic_transport.hpp>

namespace fetchpp::detail
{
basic_transport_base::basic_transport_base(std::string domain,
                                           std::uint16_t port)
  : domain_(std::move(domain)), port_(port)
{
}

std::string const& basic_transport_base::domain() const
{
  return domain_;
}

std::uint16_t basic_transport_base::port() const
{
  return port_;
}

std::string_view basic_transport_base::host() const
{
  return domain_ + ":" + std::to_string(port_);
}
}
