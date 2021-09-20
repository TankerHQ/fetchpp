#pragma once

#include <boost/beast/core/string_type.hpp>

#include <string>

namespace test::helpers
{
using string_view = boost::beast::string_view;

auto get_test_host() -> string_view;
auto get_test_proxy() -> std::string;

namespace http_literals
{
std::string operator""_http(const char* target, std::size_t);
std::string operator""_https(const char* target, std::size_t);
}
}
