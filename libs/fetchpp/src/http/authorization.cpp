#include <fetchpp/http/authorization.hpp>

#include <boost/beast/core/detail/base64.hpp>

#include <fetchpp/alias/beast.hpp>

#include <ostream>

namespace fetchpp::http::authorization
{
bearer::bearer(std::string_view token) : token_(token)
{
}

std::string const& bearer::token() const noexcept
{
  return token_;
}

std::ostream& operator<<(std::ostream& os, bearer const& bearer)
{
  os << "Bearer " << bearer.token();
  return os;
}

basic::basic(std::string_view user, std::string_view password)
  : user_(user), password_(password)
{
}

std::string const& basic::user() const noexcept
{
  return user_;
}

std::string const& basic::password() const noexcept
{
  return password_;
}

std::ostream& operator<<(std::ostream& os, basic const& b)
{
  using namespace beast::detail::base64;
  auto const source = b.user() + ':' + b.password();
  auto out = std::string(encoded_size(source.length()), '\0');
  out.resize(encode(out.data(), source.data(), source.length()));
  os << "Basic " << out;
  return os;
}

std::ostream& operator<<(std::ostream& os, methods const& m)
{
  std::visit([&os](auto&& m) { os << m; }, m);
  return os;
}

}
