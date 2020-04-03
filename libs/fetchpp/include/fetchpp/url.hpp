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

  void set_scheme(std::string);
  void set_domain(std::string);
  void set_port(uint16_t);
  void set_target(std::string);

private:
  std::string _scheme;
  std::string _domain;
  uint16_t _port;
  std::string _target;
};
}
