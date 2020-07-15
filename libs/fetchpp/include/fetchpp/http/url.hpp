#pragma once

#include <nlohmann/json_fwd.hpp>
#include <skyr/url.hpp>

#include <cstdint>
#include <string>

namespace fetchpp::http
{
using skyr::url;

bool is_ssl_involved(url const& uri);
std::int16_t safe_port(url const& uri);

nlohmann::json decode_query(std::string_view);
std::string encode_query(nlohmann::json const&);
}
