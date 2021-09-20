#include <catch2/catch.hpp>

#include <fetchpp/http/authorization.hpp>
#include <fetchpp/http/proxy.hpp>

#include <stdlib.h>

namespace http = fetchpp::http;
using namespace fetchpp::http::url_literals;

namespace helper
{
namespace
{
void setenv(fetchpp::string_view key,
            fetchpp::string_view value,
            bool overwrite)
{
#ifndef WIN32
  ::setenv(key.data(), value.data(), overwrite);
#else
  _putenv_s(key.data(), value.data());
#endif
}
}
}

TEST_CASE("parse a proxy url", "[proxy][parse]")
{
  http::proxy proxy;
  {
    REQUIRE_NOTHROW(proxy =
                        fetchpp::http::proxy::parse("https://localhost:3127"));
    CHECK(proxy.url().host() == "localhost:3127");
    CHECK(proxy.url().port() == "3127");
    CHECK(proxy.url().hostname() == "localhost");
    CHECK(proxy.url().scheme() == "https");
  }
  {
    REQUIRE_NOTHROW(proxy = fetchpp::http::proxy::parse("https://proxy.com"));
    CHECK(proxy.url().host() == "proxy.com");
    CHECK(proxy.url().port() == "");
    CHECK(proxy.url().hostname() == "proxy.com");
    CHECK(proxy.url().scheme() == "https");
  }
  {
    REQUIRE_THROWS(fetchpp::http::proxy::parse("192.168.12.21:3232"));
  }
  {
    REQUIRE_NOTHROW(
        proxy = fetchpp::http::proxy::parse("http://192.168.12.21:3232"));
    CHECK(proxy.url().host() == "192.168.12.21:3232");
    CHECK(proxy.url().port() == "3232");
    CHECK(proxy.url().hostname() == "192.168.12.21");
    CHECK(proxy.url().scheme() == "http");
  }
}

TEST_CASE("parse a proxy url with a username and a password", "[proxy][parse]")
{
  http::proxy proxy;
  {
    REQUIRE_NOTHROW(proxy = fetchpp::http::proxy::parse(
                        "https://dave:mustaine@farhost.com:3127"));
    CHECK(proxy.url().host() == "farhost.com:3127");
    CHECK(proxy.url().port() == "3127");
    CHECK(proxy.url().hostname() == "farhost.com");
    CHECK(proxy.url().scheme() == "https");
    CHECK(proxy.fields().count(http::field::proxy_authorization) == 1);
  }
}

TEST_CASE("select proxy", "[proxy][select]")
{
  http::proxy_map proxies;
  proxies.emplace(http::proxy_scheme::https, "http://firstproxy:3128");
  proxies.emplace(http::proxy_scheme::http, "http://secondproxy:3128");
  proxies.emplace("https://dev-api.farway.com", "http://thirdproxy:3128");

  {
    auto const proxy = http::select_proxy(proxies, "https://anywhere.com"_url);
    REQUIRE(proxy);
    CHECK(proxy->url() == "http://firstproxy:3128"_url);
  }
  {
    auto const proxy = http::select_proxy(proxies, "http://anywhere.com"_url);
    REQUIRE(proxy);
    CHECK(proxy->url() == "http://secondproxy:3128"_url);
  }
  {
    auto const proxy =
        http::select_proxy(proxies, "https://dev-api.farway.com"_url);
    REQUIRE(proxy);
    CHECK(proxy->url() == "http://thirdproxy:3128"_url);
  }
  {
    auto const proxy = http::select_proxy(
        proxies, "https://dev-api.farway.com/get_somewhere_else"_url);
    REQUIRE(proxy);
    CHECK(proxy->url() == "http://thirdproxy:3128"_url);
  }
  {
    auto const proxy =
        http::select_proxy(proxies, "http://dev-api.farway.com"_url);
    REQUIRE(proxy);
    CHECK(proxy->url() == "http://secondproxy:3128"_url);
  }
}
#ifndef WIN32
TEST_CASE("proxy_from_env", "[proxy][environment]")
{
  SECTION("matches proxy variables in upercases")
  {
    helper::setenv("HTTP_PROXY", "http://localhost:3128", true);
    helper::setenv("HTTPS_PROXY", "http://localhost:3129", true);
    auto proxy_map = fetchpp::http::proxy_from_environment();
    REQUIRE(proxy_map.size() == 2);
    REQUIRE(proxy_map.at(http::proxy_scheme::http).url() ==
            "http://localhost:3128"_url);
    REQUIRE(proxy_map.at(http::proxy_scheme::https).url() ==
            "http://localhost:3129"_url);
  }

  SECTION("matches lower cases variables and overrides uppercases one")
  {
    helper::setenv("http_proxy", "http://faraway:3128", true);
    helper::setenv("https_proxy", "http://faraway:3129", true);
    auto proxy_map = fetchpp::http::proxy_from_environment();
    REQUIRE(proxy_map.size() == 2);
    REQUIRE(proxy_map.at(http::proxy_scheme::http).url() ==
            "http://faraway:3128"_url);
    REQUIRE(proxy_map.at(http::proxy_scheme::https).url() ==
            "http://faraway:3129"_url);
  }
}
#endif
