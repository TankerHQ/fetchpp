#pragma once

#include <fetchpp/http/content_type.hpp>

#include <fetchpp/http/message.hpp>

#include <boost/beast/core/multi_buffer.hpp>

#include <fetchpp/alias/http.hpp>
#include <fetchpp/alias/net.hpp>

#include <nlohmann/json.hpp>

#include <optional>

namespace fetchpp::http
{
using response = message<false, beast::multi_buffer>;
}
