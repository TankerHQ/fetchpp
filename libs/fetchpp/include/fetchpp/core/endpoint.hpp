#pragma once

#include <string>
#include <type_traits>

#include <fetchpp/alias/strings.hpp>

namespace fetchpp
{
namespace detail
{
class base_endpoint
{
public:
  base_endpoint(std::string domain, std::uint16_t port);

  std::string const& domain() const;
  std::uint16_t port() const;
  std::string host() const;

private:
  std::string domain_;
  std::uint16_t port_;
};
}

template <bool isSecure>
class basic_endpoint : public detail::base_endpoint
{
  using base_t = detail::base_endpoint;

public:
  using base_t::base_t;
  using base_t::domain;
  using base_t::host;
  using base_t::port;

  using is_secure = std::integral_constant<bool, isSecure>;
};

extern template class basic_endpoint<true>;
extern template class basic_endpoint<false>;

template <bool isSecure>
extern bool operator==(basic_endpoint<isSecure> const&,
                       basic_endpoint<isSecure> const&);
template <bool isSecure>
extern bool operator!=(basic_endpoint<isSecure> const&,
                       basic_endpoint<isSecure> const&);

template <bool isSecure1, bool isSecure2>
bool operator==(basic_endpoint<isSecure1> const&,
                basic_endpoint<isSecure2> const&)
{
  return isSecure1 == isSecure2;
}

template <bool isSecure1, bool isSecure2>
bool operator!=(basic_endpoint<isSecure1> const&,
                basic_endpoint<isSecure2> const&)
{
  return isSecure1 != isSecure2;
}

using secure_endpoint = basic_endpoint<true>;
using plain_endpoint = basic_endpoint<false>;

class tunnel_endpoint
{
public:
  using is_secure = std::integral_constant<bool, true>;

  tunnel_endpoint(plain_endpoint const&, secure_endpoint const&);

  plain_endpoint const& proxy() const;
  secure_endpoint const& target() const;

private:
  plain_endpoint _proxy;
  secure_endpoint _target;
};

bool operator==(tunnel_endpoint const&, tunnel_endpoint const&);
bool operator!=(tunnel_endpoint const&, tunnel_endpoint const&);
}
