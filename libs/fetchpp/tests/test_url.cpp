#include <fetchpp/http/url.hpp>

#include <nlohmann/json.hpp>

#include <catch2/catch.hpp>

#include <fmt/format.h>

using fetchpp::http::safe_port;
using fetchpp::http::url;

TEST_CASE("default_port", "[url]")
{
  REQUIRE(url::default_port("http") == 80);
  REQUIRE(url::default_port("https") == 443);
}

TEST_CASE("parse url with port", "[url]")
{
  REQUIRE(url("https://example.com:82").port() == "82");
  REQUIRE(url("https://example.com:82").port<std::int16_t>().value() == 82);
  REQUIRE(url("https://example.com").port() == "");
  REQUIRE(url("https://example.com").port<std::int16_t>().has_value() == false);
  REQUIRE(safe_port(url("https://example.com")) == 443);
}

TEST_CASE("parse url domain", "[url]")
{
  REQUIRE(url("https://www.example.com").domain().value() == "www.example.com");
  REQUIRE(url("https://www.example.com:4242").domain().value() ==
          "www.example.com");
  REQUIRE(url("https://192.168.1.9:4242").domain().has_value() == false);
}

TEST_CASE("parse url host", "[url]")
{
  REQUIRE(url("https://www.example.com").host() == "www.example.com");
  REQUIRE(url("https://www.example.com:4242").host() == "www.example.com:4242");
  REQUIRE(url("https://192.168.1.9").host() == "192.168.1.9");
  REQUIRE(url("https://192.168.1.9:4242").host() == "192.168.1.9:4242");
}

TEST_CASE("parse url hostname", "[url]")
{
  REQUIRE(url("https://www.example.com").hostname() == "www.example.com");
  REQUIRE(url("https://www.example.com:4242").hostname() == "www.example.com");
  REQUIRE(url("https://192.168.1.9:4242").hostname() == "192.168.1.9");
  REQUIRE(url("https://192.168.1.9").hostname() == "192.168.1.9");
}

TEST_CASE("parse url protocol", "[url]")
{
  REQUIRE(url("https://www.example.com").protocol() == "https:");
  REQUIRE(url("http://www.example.com").protocol() == "http:");
  REQUIRE(url("wss://www.example.com").protocol() == "wss:");
}

TEST_CASE("build an url", "[url]")
{
  auto uri = url("Mexico/Mexico_City?temperature=celcius",
                 url("https://www.vacances.com"));
  REQUIRE(uri.protocol() == "https:");
  REQUIRE(uri.href() ==
          "https://www.vacances.com/Mexico/Mexico_City?temperature=celcius");
}

TEST_CASE("encode url query", "[url][query]")
{
  auto const uri = url("https://www.example.com?a=b&e=f&c=d&e=z");
  auto j = fetchpp::http::decode_query(uri.search());
  REQUIRE(j["a"] == "b");
  REQUIRE(j["c"] == "d");
  REQUIRE(j["e"] == nlohmann::json({"f", "z"}));

  REQUIRE(uri.search_parameters().get("a").value() == "b");
  REQUIRE(uri.search_parameters().get("c").value() == "d");
  REQUIRE(uri.search_parameters().get_all("e") ==
          std::vector<std::string>({"f", "z"}));
}

TEST_CASE("decode url query", "[url][query]")
{
  auto query = nlohmann::json({{"a", "b"}, {"c", "d"}, {"e", {"f", "z"}}});
  auto const result = fetchpp::http::encode_query(query);
  auto uri = url("https://www.example.com");
  uri.set_search(result);
  uri.search_parameters().sort();
  REQUIRE(uri == url("https://www.example.com?a=b&c=d&e=f&e=z"));
}