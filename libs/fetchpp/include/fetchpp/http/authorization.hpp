#pragma once

#include <iosfwd>
#include <string>

#include <fetchpp/alias/strings.hpp>

#include <boost/variant2/variant.hpp>

namespace fetchpp::http::authorization
{
class bearer
{
  std::string token_;

public:
  bearer() = delete;
  bearer(string_view token);
  std::string const& token() const noexcept;
};
std::string to_string(bearer const&);

class basic
{
  std::string user_;
  std::string password_;

public:
  basic() = delete;
  basic(string_view user, string_view password);

  std::string const& user() const noexcept;
  std::string const& password() const noexcept;
};
std::string to_string(basic const&);

using methods = boost::variant2::variant<basic, bearer>;

std::string to_string(methods const&);
}
