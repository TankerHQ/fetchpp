#pragma once

#include <boost/beast/http/fields.hpp>
#include <fetchpp/http/url.hpp>

#include <boost/container/flat_map.hpp>
#include <boost/variant2/variant.hpp>

#include <fetchpp/alias/http.hpp>

namespace fetchpp::http
{
enum class proxy_scheme
{
  http,
  https,
  all,
};

class proxy
{
  http::url url_;
  http::fields fields_;

  proxy(http::url, http::fields);

public:
  proxy() = default;
  static proxy parse(std::string_view);

  proxy(std::string_view url);
  http::url const& url() const;
  http::fields const& fields() const;
};

using proxy_match = boost::variant2::variant<proxy_scheme, std::string>;
using proxy_map = boost::container::flat_map<proxy_match, proxy>;

std::optional<proxy> select_proxy(proxy_map const& proxy, http::url const& url);

proxy_map proxy_from_environment();
}
