#include <fetchpp/http/authorization.hpp>

#include <boost/beast/core/detail/base64.hpp>

#include <fetchpp/alias/beast.hpp>

namespace fetchpp::http::authorization
{
bearer::bearer(std::string_view token) : token_(token)
{
}

std::string const& bearer::token() const noexcept
{
  return token_;
}

std::string to_string(bearer const& bearer)
{
  return std::string("Bearer ") + bearer.token();
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

std::string to_string(basic const& b)
{
  using namespace beast::detail::base64;
  auto const source = b.user() + ':' + b.password();
  auto out = std::string(encoded_size(source.length()), '\0');
  out.resize(encode(out.data(), source.data(), source.length()));
  return std::string("Basic ") + out;
}

std::string to_string(methods const& m)
{
  return boost::variant2::visit([](auto&& m) { return to_string(m); }, m);
}

}
