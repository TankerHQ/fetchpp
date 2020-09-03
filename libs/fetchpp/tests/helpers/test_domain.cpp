#include "test_domain.hpp"

#include <cstdlib>

#include <fmt/format.h>

namespace test::helpers
{
namespace
{
using namespace std::literals::string_view_literals;

auto constexpr DEFAULT_TEST_HOST = "httpbin.org"sv;
auto constexpr DEFAULT_TEST_PROXY = "172.17.0.1:3128"sv;

inline auto get_safe_env(std::string_view key, std::string_view default_value)
{
  if (auto env = std::getenv(key.data()); env == nullptr)
    return std::string{default_value};
  else
    return std::string{env};
}
}

std::string_view get_test_host()
{
  static auto const test_url =
      get_safe_env("FETCHPP_TEST_HOST", DEFAULT_TEST_HOST);
  return test_url;
}
std::string get_test_proxy()
{
  static auto const proxy_url = fmt::format(
      "http://{}", get_safe_env("FETCHPP_TEST_PROXY", DEFAULT_TEST_PROXY));
  return proxy_url;
}
namespace http_literals
{
std::string operator""_http(const char* target, std::size_t)
{
  return fmt::format("http://{}/{}", get_test_host(), target);
}
std::string operator""_https(const char* target, std::size_t)
{
  return fmt::format("https://{}/{}", get_test_host(), target);
}

}

}
