#include <fetchpp/http/content_type.hpp>

#include <fetchpp/alias/strings.hpp>

#include <boost/algorithm/string/predicate.hpp>

#include <regex>

namespace fetchpp::http
{
content_type::content_type(string_view type,
                           string_view charset,
                           string_view boundary)
  : base_t{std::string(type), std::string(charset), std::string(boundary)}
{
}

content_type::content_type(std::string&& type,
                           std::string&& charset,
                           std::string&& boundary)
  : base_t{std::move(type), std::move(charset), std::move(boundary)}
{
}

content_type content_type::parse(string_view sv)
{
  // https://regex101.com/r/JGZNn8/2/
  static auto const reg = std::regex(
      R"(^([\w!#$&\-^]{1,127}?\/[\w!#$&\-^]{1,127}?)(?:;\s*charset=([\w!#$%^&\-~{}]{1,127}?))?(?:;\s*boundary=([\w!#$%^&\-~{}]{1,70}?))?\s*$)",
      std::regex_constants::icase | std::regex_constants::optimize);
  std::smatch match;
  auto const str = std::string{sv};
  if (!std::regex_match(str.cbegin(), str.cend(), match, reg))
    throw std::domain_error("malformed content type");
  return content_type(match[1], match[2], match[3]);
}

std::string const& content_type::type() const
{
  return std::get<0>(*this);
}

void content_type::type(string_view t)
{
  std::get<0>(*this) = t.to_string();
}

std::string const& content_type::charset() const
{
  return std::get<1>(*this);
}

void content_type::charset(string_view c)
{
  std::get<1>(*this) = c.to_string();
}

std::string const& content_type::boundary() const
{
  return std::get<2>(*this);
}

void content_type::boundary(string_view b)
{
  std::get<2>(*this) = b.to_string();
}

bool operator==(content_type const& lhs, content_type const& rhs)
{
  return boost::iequals(lhs.type(), rhs.type()) &&
         boost::iequals(lhs.charset(), rhs.charset()) &&
         lhs.boundary() == rhs.boundary();
}

bool operator!=(content_type const& lhs, content_type const& rhs)
{
  return !(lhs == rhs);
}

std::string to_string(content_type const& ct)
{
  std::string str(ct.type());
  if (!ct.charset().empty())
    str + "; charset=" + ct.charset();
  if (!ct.boundary().empty())
    str + "; boundary=" + ct.boundary();
  return str;
}
}
