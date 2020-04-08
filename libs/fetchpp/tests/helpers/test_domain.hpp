#pragma once

#include <string>
#include <string_view>

namespace test::helpers
{
auto get_test_host() -> std::string_view;

namespace http_literals
{
std::string operator""_http(const char* target, std::size_t);
std::string operator""_https(const char* target, std::size_t);
}
}
