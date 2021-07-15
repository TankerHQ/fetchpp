#include <fetchpp/core/detail/endpoint.hpp>

namespace fetchpp
{
namespace detail
{
base_endpoint::base_endpoint(std::string domain, std::uint16_t port)
  : domain_(std::move(domain)), port_(port)
{
}

std::string const& base_endpoint::domain() const
{
  return domain_;
}

std::uint16_t base_endpoint::port() const
{
  return port_;
}

std::string base_endpoint::host() const
{
  return domain_ + ":" + std::to_string(port_);
}
}

template <bool isSecure>
bool operator==(basic_endpoint<isSecure> const& lhs,
                basic_endpoint<isSecure> const& rhs)
{
  return lhs.port() == rhs.port() && lhs.domain() == rhs.domain();
}
template bool operator==
    <true>(basic_endpoint<true> const&, basic_endpoint<true> const&);
template bool operator==
    <false>(basic_endpoint<false> const&, basic_endpoint<false> const&);

template <bool isSecure>
bool operator!=(basic_endpoint<isSecure> const& lhs,
                basic_endpoint<isSecure> const& rhs)
{
  return !(lhs == rhs);
}
template bool operator!=
    <true>(basic_endpoint<true> const&, basic_endpoint<true> const&);
template bool operator!=
    <false>(basic_endpoint<false> const&, basic_endpoint<false> const&);

template class basic_endpoint<true>;
template class basic_endpoint<false>;

tunnel_endpoint::tunnel_endpoint(plain_endpoint const& proxy,
                                 secure_endpoint const& target)
  : _proxy(proxy), _target(target)
{
}

plain_endpoint const& tunnel_endpoint::proxy() const
{
  return _proxy;
}

secure_endpoint const& tunnel_endpoint::target() const
{
  return _target;
}

bool operator==(tunnel_endpoint const& left, tunnel_endpoint const& right)
{
  return left.proxy() == right.proxy() && left.target() == right.target();
}

bool operator!=(tunnel_endpoint const& left, tunnel_endpoint const& right)
{
  return !(left == right);
}
}
