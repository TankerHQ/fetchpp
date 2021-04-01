#pragma once

#include <iosfwd>
#include <string>
#include <string_view>

#include <boost/variant2/variant.hpp>

namespace fetchpp::http::authorization
{
class bearer
{
  std::string token_;

public:
  bearer() = delete;
  bearer(std::string_view token);
  std::string const& token() const noexcept;
};
std::ostream& operator<<(std::ostream& os, bearer const& ct);
std::string to_string(bearer const&);

class basic
{
  std::string user_;
  std::string password_;

public:
  basic() = delete;
  basic(std::string_view user, std::string_view password);

  std::string const& user() const noexcept;
  std::string const& password() const noexcept;
};
std::ostream& operator<<(std::ostream& os, basic const& ct);
std::string to_string(basic const&);

using methods = boost::variant2::variant<basic, bearer>;

std::ostream& operator<<(std::ostream& os, methods const& m);
std::string to_string(methods const&);
}
