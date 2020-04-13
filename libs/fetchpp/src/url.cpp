#include <fetchpp/url.hpp>

#include <exception>
#include <regex>

namespace fetchpp
{
namespace
{
template <typename T>
uint16_t service_to_port(std::basic_string<T> const& service)
{
  // maybe use asio resoler for that ? what about async ?
  if (service == "https")
    return 443;
  else if (service == "http")
    return 80;
  else
    return 0;
}
template <typename T, std::size_t N>
uint16_t service_to_port(T const (&service)[N])
{
  if (std::strcmp(service, "https"))
    return 443;
  else if (std::strcmp(service, "http"))
    return 80;
  return 0;
}

template <typename T>
uint16_t match_port(std::sub_match<T> const& port_match,
                    std::sub_match<T> const& scheme_match)
{
  if (port_match.matched)
    return std::stoi(port_match.str());
  return service_to_port(scheme_match.str());
}
}

url url::parse(std::string const& purl)
{
  using namespace std::string_literals;
  // https://regex101.com/r/QfqRd0/5
  static auto const rg = std::regex(
      R"(^(?:(https?)://)?(?:.*?@)?([^:@/]+?)(?::(\d+))?([/?].*?)?$)",
      std::regex::optimize);
  std::smatch match;
  if (!std::regex_match(purl, match, rg))
    throw std::runtime_error("bad url");
  if (!match[2].matched)
    throw std::runtime_error("no domain!");

  return {match[1], match[2], match_port(match[3], match[1]), match[4]};
}

url::url(std::string scheme,
         std::string domain,
         uint16_t port,
         std::string target)
  : _scheme(std::move(scheme)),
    _domain(std::move(domain)),
    _port(port),
    _target(std::move(target))
{
}

std::string const& url::scheme() const
{
  return _scheme;
}

std::string const& url::domain() const
{
  return _domain;
}

uint16_t url::port() const
{
  return _port;
}

std::string const& url::target() const
{
  return _target;
}

void url::scheme(std::string_view scheme)
{
  _scheme = scheme;
}

void url::domain(std::string_view domain)
{
  _domain = domain;
}

void url::port(uint16_t p)
{
  _port = p;
}

void url::target(std::string_view target)
{
  _target = target;
}

namespace http_literals
{
url operator""_https(const char* target, std::size_t)
{
  auto res = url::parse(target);
  res.scheme("https");
  res.port(service_to_port("https"));
  return res;
}
url operator""_http(const char* target, std::size_t)
{
  auto res = url::parse(target);
  res.scheme("http");
  res.port(service_to_port("http"));
  return res;
}
}

}
