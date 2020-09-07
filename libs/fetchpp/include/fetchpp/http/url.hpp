#pragma once

#include <nlohmann/json_fwd.hpp>
#include <skyr/url.hpp>

#include <cstdint>
#include <string>

namespace fetchpp::http
{

class url : public skyr::url
{
  using base_t = skyr::url;

public:
  using string_type = base_t::string_type;
  using skyr::url::url;
  string_type target() const;
};

bool is_ssl_involved(url const& uri);
std::int16_t safe_port(url const& uri);

nlohmann::json decode_query(std::string_view);
std::string encode_query(nlohmann::json const&);

namespace url_literals
{
inline auto operator "" _url(const char *str, std::size_t length) {
  return url(std::string_view(str, length));
}
}
}
