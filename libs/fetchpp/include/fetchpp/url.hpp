#pragma once

#include <string>

namespace fetchpp
{
class url
{
public:
  url(std::string scheme,
      std::string domain,
      uint16_t port,
      std::string target);
  static url parse(std::string const& url);

  std::string const& scheme() const;
  std::string const& domain() const;
  uint16_t port() const;
  std::string const& target() const;

  void scheme(std::string_view);
  void domain(std::string_view);
  void port(uint16_t);
  void target(std::string_view);

private:
  std::string _scheme;
  std::string _domain;
  uint16_t _port;
  std::string _target;
};

namespace http_literals
{
url operator""_https(const char* target, std::size_t);
url operator""_http(const char* target, std::size_t);
}
}
