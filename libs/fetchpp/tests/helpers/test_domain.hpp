#pragma once

#include <fmt/format.h>

#include <string>

namespace test::helpers::http_literals
{
static auto constexpr TEST_URL = "httpbin.org";

inline std::string operator""_http(const char* target, std::size_t)
{
  return fmt::format("http://{}/{}", TEST_URL, target);
}

inline std::string operator""_https(const char* target, std::size_t)
{
  return fmt::format("https://{}/{}", TEST_URL, target);
}

}
