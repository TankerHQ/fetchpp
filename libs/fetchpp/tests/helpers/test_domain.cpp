#include "test_domain.hpp"

#include <cstdlib>

#include <fmt/format.h>

namespace test::helpers
{
namespace
{

auto constexpr DEFAULT_TEST_HOST = string_view{"httpbin.org"};
auto constexpr DEFAULT_TEST_PROXY = string_view{"172.17.0.1:3128"};

inline auto get_safe_env(string_view key, string_view default_value)
{
  if (auto env = std::getenv(key.data()); env == nullptr)
    return std::string{default_value};
  else
    return std::string{env};
}
}

string_view get_test_host()
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
namespace
{
std::string do_format(char const* scheme, char const* target)
{
  auto str = get_test_host();
  return fmt::format(
      "{}://{}/{}", scheme, std::string_view{str.data(), str.size()}, target);
}
}
std::string operator""_http(const char* target, std::size_t)
{
  return do_format("http", target);
}
std::string operator""_https(const char* target, std::size_t)
{
  return do_format("https", target);
}

}

}
