#include <fetchpp/url.hpp>

#include <exception>
#include <regex>

namespace fetchpp
{
url url::parse(std::string const& purl)
{
  using namespace std::string_literals;
  // https://regex101.com/r/qrDzR5/6
  static auto const rg = std::regex(
      R"((?:^(https?)://)?(?:.*?@)?(.+(?:\..+?)+?)(?::(\d+))?([/?].*?)?$)",
      std::regex::optimize);
  std::smatch match;
  if (!std::regex_match(purl, match, rg))
    throw std::runtime_error("bad url");
  uint16_t port = std::stoi(
      match[3].matched ? match[3].str() :
                         (match[1].compare("https") == 0 ? "443"s : "80"s));
  if (!match[2].matched)
    throw std::runtime_error("no domain!");

  return {match[1], match[2], port, match[4]};
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

void url::set_scheme(std::string scheme)
{
  _scheme = std::move(scheme);
}

void url::set_domain(std::string domain)
{
  _domain = std::move(domain);
}

void url::set_port(uint16_t p)
{
  _port = p;
}

void url::set_target(std::string target)
{
  _target = std::move(target);
}
}
