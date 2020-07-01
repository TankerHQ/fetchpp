#pragma once

#include <nlohmann/json_fwd.hpp>
#include <skyr/url.hpp>

#include <cstdint>
#include <string>

namespace fetchpp::http
{
using skyr::url;
namespace http_literals
{
url operator""_https(const char* target, std::size_t);
url operator""_http(const char* target, std::size_t);
}

bool is_ssl_involved(url const& uri);
std::int16_t safe_port(url const& uri);

nlohmann::json decode_query(std::string_view);
std::string encode_query(nlohmann::json const&);
}
