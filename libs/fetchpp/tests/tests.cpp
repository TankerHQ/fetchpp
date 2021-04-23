#include <catch2/catch.hpp>

#include <fetchpp/fetch.hpp>
#include <fetchpp/get.hpp>
#include <fetchpp/post.hpp>
#include <fetchpp/version.hpp>

#include <fetchpp/http/authorization.hpp>
#include <fetchpp/http/content_type.hpp>
#include <fetchpp/http/response.hpp>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/use_future.hpp>

#include "helpers/format.hpp"
#include "helpers/ioc_fixture.hpp"
#include "helpers/test_domain.hpp"

#include <nlohmann/json.hpp>

using namespace test::helpers::http_literals;
using test::helpers::ioc_fixture;

namespace
{
auto EqualsHeader(nlohmann::json const& headers, std::string const& field)
{
  using Catch::Matchers::Equals;
  return Equals(headers.at(field).get<std::string>(), Catch::CaseSensitive::No);
}
}

TEST_CASE_METHOD(ioc_fixture,
                 "async fetch headers with lambda",
                 "[https][fetch][lambda]")
{
  auto request = fetchpp::http::request(fetchpp::http::verb::get,
                                        fetchpp::http::url("get"_https));
  request.set("x-random-header", "this is a cute value");
  fetchpp::async_fetch(ex, std::move(request), [](auto err, auto response) {
    REQUIRE(!err);
    REQUIRE(response.result_int() == 200);
    REQUIRE(response.is_json());
    auto const jsonbody = response.json();
    auto const& headers = jsonbody.at("headers");
    CHECK_THAT("this is a cute value",
               EqualsHeader(headers, "X-Random-Header"));
    CHECK_THAT("no-cache", EqualsHeader(headers, "Cache-Control"));
    CHECK_THAT(fetchpp::USER_AGENT, EqualsHeader(headers, "User-Agent"));
  });
}

TEST_CASE_METHOD(ioc_fixture, "async fetch headers", "[https][fetch]")
{
  auto request = fetchpp::http::request(fetchpp::http::verb::get,
                                        fetchpp::http::url("get"_https));
  request.set("X-Random-Header", "this is a cute value");
  auto response =
      fetchpp::async_fetch(ex, std::move(request), boost::asio::use_future)
          .get();
  REQUIRE(response.result_int() == 200);
  REQUIRE(response.is_json());
  auto const jsonbody = response.json();
  auto const& headers = jsonbody.at("headers");
  CHECK_THAT("this is a cute value", EqualsHeader(headers, "X-Random-Header"));
  CHECK_THAT("no-cache", EqualsHeader(headers, "Cache-Control"));
  CHECK_THAT(fetchpp::USER_AGENT, EqualsHeader(headers, "User-Agent"));
}

TEST_CASE_METHOD(ioc_fixture,
                 "async fetch beast::http::response headers with lambda",
                 "[https][fetch][lambda]")
{
  auto request = fetchpp::http::request(fetchpp::http::verb::get,
                                        fetchpp::http::url("get"_https));
  request.set("X-Random-Header", "this is a cute value");
  fetchpp::async_fetch(ex, std::move(request), [](auto err, auto response) {
    REQUIRE(!err);
    REQUIRE(response.result_int() == 200);
    REQUIRE(response.is_json());
    auto const jsonbody = response.json();
    auto const& headers = jsonbody.at("headers");
    CHECK_THAT("this is a cute value",
               EqualsHeader(headers, "X-Random-Header"));
    CHECK_THAT("no-cache", EqualsHeader(headers, "Cache-Control"));
    CHECK_THAT(fetchpp::USER_AGENT, EqualsHeader(headers, "User-Agent"));
  });
}

TEST_CASE_METHOD(ioc_fixture, "async fetch through https", "[https][fetch]")
{
  auto request = fetchpp::http::request(fetchpp::http::verb::get,
                                        fetchpp::http::url("get"_https));
  auto response =
      fetchpp::async_fetch(ex, std::move(request), boost::asio::use_future)
          .get();
  REQUIRE(response.result_int() == 200);
  REQUIRE(response.is_json());
  auto const jsonbody = response.json();
  auto const& headers = jsonbody.at("headers");
  CHECK_THAT("no-cache", EqualsHeader(headers, "Cache-Control"));
  CHECK_THAT(fetchpp::USER_AGENT, EqualsHeader(headers, "User-Agent"));
}

TEST_CASE_METHOD(ioc_fixture, "async fetch", "[https][fetch]")
{
  auto request = fetchpp::http::request(fetchpp::http::verb::get,
                                        fetchpp::http::url("get"_https));
  request.set("X-Random-Header", "this is a cute value");
  auto response =
      fetchpp::async_fetch(ex, std::move(request), boost::asio::use_future)
          .get();
  REQUIRE(response.result_int() == 200);
  REQUIRE(response.is_json());
  REQUIRE(!response.is_text());

  SECTION("use body as text")
  {
    REQUIRE_NOTHROW(response.text());
    auto json = nlohmann::json::parse(response.text());
    auto const& headers = json.at("headers");
    REQUIRE_THAT("this is a cute value",
                 EqualsHeader(headers, "X-Random-Header"));
  }

  SECTION("use body as json")
  {
    auto const json = response.json();
    auto const& headers = json.at("headers");
    REQUIRE_THAT("this is a cute value",
                 EqualsHeader(headers, "X-Random-Header"));
  }

  SECTION("use body as data")
  {
    auto body = fetchpp::beast::buffers_to_string(response.content());
    auto json = nlohmann::json::parse(body);
    auto const& headers = json.at("headers");
    REQUIRE_THAT("this is a cute value",
                 EqualsHeader(headers, "X-Random-Header"));
  }
}

TEST_CASE_METHOD(ioc_fixture,
                 "async fetch basic auth",
                 "[https][fetch][basic_arth][authorization]")
{
  auto request = fetchpp::http::request(
      fetchpp::http::verb::get,
      fetchpp::http::url("basic-auth/david/totopaf"_https));
  request.set(fetchpp::http::authorization::basic("david", "totopaf"));
  auto response =
      fetchpp::async_fetch(
          ioc.get_executor(), std::move(request), boost::asio::use_future)
          .get();

  REQUIRE(response.result_int() == 200);
}

TEST_CASE_METHOD(ioc_fixture,
                 "async fetch bearer auth",
                 "[https][fetch][bearer_auth][authorization]")
{
  auto request = fetchpp::http::request(fetchpp::http::verb::get,
                                        fetchpp::http::url("bearer"_https));
  request.set(
      fetchpp::http::authorization::bearer("this_is_a_bearer_token_probably"));
  auto response =
      fetchpp::async_fetch(
          ioc.get_executor(), std::move(request), boost::asio::use_future)
          .get();

  REQUIRE(response.result_int() == 200);
}

TEST_CASE_METHOD(ioc_fixture,
                 "async fetch with ssl context",
                 "[https][fetch][get][verify_peer][!mayfail]")
{
  auto request = fetchpp::http::request(fetchpp::http::verb::get,
                                        fetchpp::http::url("get"_https));
  request.set("X-Random-Header", "this is a cute value");

  using fetchpp::net::ssl::context;
  auto sslc = context{context::tls_client};
  // We need this for some broken build of boost ssl backend
  sslc.add_verify_path("/etc/ssl/certs");
  sslc.add_verify_path("/etc/ssl/private");
  sslc.set_verify_mode(context::verify_peer);
  auto response = fetchpp::async_fetch(
                      ex, sslc, std::move(request), boost::asio::use_future)
                      .get();
  REQUIRE(response.result_int() == 200);
  REQUIRE(response.is_json());
  auto const jsonbody = response.json();
  auto const& headers = jsonbody.at("headers");
  REQUIRE_THAT("this is a cute value",
               EqualsHeader(headers, "X-Random-Header"));
}

TEST_CASE_METHOD(ioc_fixture, "http async fetch with header", "[http][get]")
{
  auto request = fetchpp::http::request(fetchpp::http::verb::get,
                                        fetchpp::http::url("get"_http));
  request.set("X-Random-Header", "this is a cute value");
  auto response =
      fetchpp::async_fetch(ex, std::move(request), boost::asio::use_future)
          .get();
  REQUIRE(response.result_int() == 200);
  REQUIRE(response.is_json());
  auto const json = response.json();
  REQUIRE_THAT("this is a cute value",
               EqualsHeader(json.at("headers"), "X-Random-Header"));
}

TEST_CASE_METHOD(ioc_fixture, "http async get", "[https][get]")
{
  auto const url = GENERATE("get"_http, "get"_https);
  INFO("requesting " << url);
  auto response =
      fetchpp::async_get(
          ex,
          url,
          fetchpp::http::headers{{"X-random-header", "this is a cute value"}},
          boost::asio::use_future)
          .get();
  REQUIRE(response.result_int() == 200);
  REQUIRE(response.is_json());
  auto const json = response.json();
  REQUIRE_THAT("this is a cute value",
               EqualsHeader(json.at("headers"), "X-Random-Header"));
}

TEST_CASE_METHOD(ioc_fixture,
                 "http async get with lambda",
                 "[https][get][lambda]")
{
  fetchpp::async_get(
      ex,
      "get"_https,
      fetchpp::http::headers{{"X-random-header", "this is a cute value"}},
      [](auto err, fetchpp::http::response response) {
        REQUIRE(!err);
        REQUIRE(response.result_int() == 200);
        REQUIRE(response.is_json());
        auto const jsonbody = response.json();
        REQUIRE_THAT("this is a cute value",
                     EqualsHeader(jsonbody.at("headers"), "X-Random-Header"));
      });
}

TEST_CASE_METHOD(ioc_fixture, "async post string", "[https][post]")
{
  auto const data = std::string("this is my data");
  auto response = fetchpp::async_post(
                      ex,
                      "post"_https,
                      fetchpp::net::buffer(data),
                      fetchpp::http::headers{{"X-corp-header", "corp value "}},
                      boost::asio::use_future)
                      .get();
  REQUIRE(response.result_int() == 200);
  REQUIRE(response.is_json());
  auto const& json = response.json();
  REQUIRE(json.at("headers").at("X-Corp-Header") == "corp value");
  REQUIRE(json.at("data") == data);
}

TEST_CASE_METHOD(ioc_fixture, "async post json", "[https][post][json]")
{
  auto body = nlohmann::json({{"a key", "a value"}});
  auto response = fetchpp::async_post(
                      ex,
                      "post"_https,
                      fetchpp::net::buffer(body.dump()),
                      fetchpp::http::headers{{"X-corp-header", "corp value "}},
                      boost::asio::use_future)
                      .get();
  REQUIRE(response.result_int() == 200);
  auto const& json = response.json();
  CHECK(json.at("headers").at("X-Corp-Header") == "corp value");
  CHECK(json.at("json") == nlohmann::json({{"a key", "a value"}}));
}
