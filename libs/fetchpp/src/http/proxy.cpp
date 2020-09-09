#include <fetchpp/http/proxy.hpp>

#include <fetchpp/http/authorization.hpp>

#include <skyr/core/parse.hpp>

#include <cstdlib>

namespace fetchpp::http
{
namespace
{
proxy_scheme scheme_to_proxy_scheme(std::string_view scheme)
{
  if (scheme == "https")
    return proxy_scheme::https;
  else if (scheme == "http")
    return proxy_scheme::http;
  else if (scheme == "all")
    return proxy_scheme::all;
  throw std::runtime_error("bad proxy scheme");
}
std::optional<std::string> safe_get_env(
    std::string_view key, std::optional<std::string> def = std::nullopt)
{
  auto value = std::getenv(key.data());
  if (value != nullptr && std::strlen(value) > 0)
    return std::string(value);
  else
    return def;
}
}
std::optional<proxy> select_proxy(proxy_map const& proxies,
                                  http::url const& url)
{
  std::vector<proxy_match> keys{url.origin(),
                                scheme_to_proxy_scheme(url.scheme())};
  for (auto const& key : keys)
  {
    if (auto it = proxies.find(key); it != proxies.end())
      return it->second;
  }
  return std::nullopt;
}

proxy_map proxy_from_environment()
{
  proxy_map proxies;
  if (auto const http_proxy =
          safe_get_env("http_proxy", safe_get_env("HTTP_PROXY"));
      http_proxy.has_value())
    proxies.emplace(fetchpp::http::proxy_scheme::http, http_proxy.value());
  if (auto const https_proxy =
          safe_get_env("https_proxy", safe_get_env("HTTPS_PROXY"));
      https_proxy.has_value())
    proxies.emplace(fetchpp::http::proxy_scheme::https, https_proxy.value());
  return proxies;
}

proxy proxy::parse(std::string_view sv)
{
  auto expected = skyr::parse(sv);
  if (!expected.has_value())
    throw std::runtime_error("cannot parse proxy url");
  auto record = expected.value();
  if (!record.host.has_value() || record.scheme.empty())
    throw std::runtime_error("invalid proxy url");
  http::fields fields;
  if (!record.username.empty() && !record.password.empty())
    fields.set(field::proxy_authorization,
               authorization::basic(record.username, record.password));
  return proxy{http::url(std::move(record)), std::move(fields)};
}

proxy::proxy(http::url url, http::fields fields)
  : url_(std::move(url)), fields_(std::move(fields))
{
}

proxy::proxy(std::string_view sv) : proxy(parse(sv))
{
}

http::url const& proxy::url() const
{
  return url_;
}

http::fields const& proxy::fields() const
{
  return fields_;
}
}
