#include <fetchpp/content_type.hpp>

#include <boost/algorithm/string/predicate.hpp>
#include <regex>

namespace fetchpp
{
content_type::content_type(std::string type,
                           std::string charset,
                           std::string boundary)
  : base_t{std::move(type), std::move(charset), std::move(boundary)}
{
}

content_type content_type::parse(boost::string_view sv)
{
  return content_type::parse(std::string_view{sv.data(), sv.size()});
}

content_type content_type::parse(std::string_view sv)
{
  // https://regex101.com/r/JGZNn8/2/
  static auto const reg = std::regex(
      R"(^([\w!#$&\-^]{1,127}?\/[\w!#$&\-^]{1,127}?)(?:;\s*charset=([\w!#$%^&\-~{}]{1,127}?))?(?:;\s*boundary=([\w!#$%^&\-~{}]{1,70}?))?\s*$)",
      std::regex_constants::icase | std::regex_constants::optimize);
  std::smatch match;
  auto const str = std::string{sv};
  if (!std::regex_match(str.cbegin(), str.cend(), match, reg))
    throw std::domain_error("malformed content type");
  return content_type{match[1], match[2], match[3]};
}

std::string const& content_type::type() const
{
  return std::get<0>(*this);
}

void content_type::type(std::string_view t)
{
  std::get<0>(*this) = t;
}

std::string const& content_type::charset() const
{
  return std::get<1>(*this);
}

void content_type::charset(std::string_view c)
{
  std::get<1>(*this) = c;
}

std::string const& content_type::boundary() const
{
  return std::get<2>(*this);
}

void content_type::boundary(std::string_view b)
{
  std::get<2>(*this) = b;
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

std::ostream& operator<<(std::ostream& os, content_type const& ct)
{
  os << ct.type();
  if (!ct.charset().empty())
    os << "; charset=" << ct.charset();
  if (!ct.boundary().empty())
    os << "; boundary=" << ct.boundary();
  return os;
}
}
