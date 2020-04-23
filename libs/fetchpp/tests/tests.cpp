#include <catch2/catch.hpp>

#include <fetchpp/fetch.hpp>
#include <fetchpp/get.hpp>
#include <fetchpp/post.hpp>

#include <fetchpp/http/authorization.hpp>
#include <fetchpp/http/content_type.hpp>
#include <fetchpp/http/json_body.hpp>
#include <fetchpp/http/response.hpp>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/use_future.hpp>

#include "helpers/format.hpp"
#include "helpers/ioc_fixture.hpp"
#include "helpers/test_domain.hpp"

#include <nlohmann/json.hpp>

using namespace test::helpers::http_literals;
using test::helpers::ioc_fixture;

template <typename T>
using Response = fetchpp::beast::http::response<T>;
template <typename T>
using Request = fetchpp::request<T>;

using StringResponse = Response<fetchpp::http::string_body>;
using JsonResponse = Response<fetchpp::http::json_body>;

using StringRequest = Request<fetchpp::http::string_body>;
using JsonRequest = Request<fetchpp::http::json_body>;
namespace
{
auto EqualsHeader(nlohmann::json const& headers, std::string const& field)
{
  using Catch::Matchers::Equals;
  return Equals(headers.at(field).get<std::string>(), Catch::CaseSensitive::No);
}
}

TEST_CASE_METHOD(ioc_fixture,
                 "async fetch beast::http::response headers",
                 "[https][fetch][async]")
{
  auto request =
      make_request(fetchpp::http::verb::get, fetchpp::url::parse("get"_https));
  request.set("X-Random-Header", "this is a cute value");
  auto response = fetchpp::async_fetch<JsonResponse>(
                      ioc, std::move(request), boost::asio::use_future)
                      .get();
  REQUIRE(response.result_int() == 200);
  auto const content_type =
      fetchpp::content_type::parse(response.at(fetchpp::field::content_type));
  REQUIRE(response.at(fetchpp::field::content_type) == "application/json");
  auto const& jsonBody = response.body();
  auto const& headers = jsonBody.at("headers");
  CHECK_THAT("this is a cute value", EqualsHeader(headers, "X-Random-Header"));
  CHECK_THAT("no-cache", EqualsHeader(headers, "Cache-Control"));
  CHECK_THAT(fetchpp::USER_AGENT, EqualsHeader(headers, "User-Agent"));
}

TEST_CASE_METHOD(ioc_fixture, "async fetch headers", "[https][fetch][async]")
{
  auto request =
      make_request(fetchpp::http::verb::get, fetchpp::url::parse("get"_https));
  request.set("X-Random-Header", "this is a cute value");
  auto response =
      fetchpp::async_fetch(ioc, std::move(request), boost::asio::use_future)
          .get();
  REQUIRE(response.result_int() == 200);
  auto const content_type =
      fetchpp::content_type::parse(response.at(fetchpp::field::content_type));
  REQUIRE(response.at(fetchpp::field::content_type) == "application/json");
  auto const& jsonBody = response.json();
  auto const& headers = jsonBody.at("headers");
  CHECK_THAT("this is a cute value", EqualsHeader(headers, "X-Random-Header"));
  CHECK_THAT("no-cache", EqualsHeader(headers, "Cache-Control"));
  CHECK_THAT(fetchpp::USER_AGENT, EqualsHeader(headers, "User-Agent"));
}

TEST_CASE_METHOD(ioc_fixture, "async fetch", "[https][fetch][async]")
{
  auto request =
      make_request(fetchpp::http::verb::get, fetchpp::url::parse("get"_https));
  request.set("X-Random-Header", "this is a cute value");
  auto response =
      fetchpp::async_fetch(ioc, std::move(request), boost::asio::use_future)
          .get();
  REQUIRE(response.result_int() == 200);
  REQUIRE(response.content_type().type() == "application/json");
  REQUIRE(response.is_json());
  REQUIRE(!response.is_text());

  SECTION("use body as json")
  {
    REQUIRE_NOTHROW(response.json());
    auto json = response.json();
    auto const& headers = json.at("headers");
    REQUIRE_THAT("this is a cute value",
                 EqualsHeader(headers, "X-Random-Header"));
  }

  SECTION("use body as text")
  {
    REQUIRE_NOTHROW(response.text());
    auto json = nlohmann::json::parse(response.text());
    auto const& headers = json.at("headers");
    REQUIRE_THAT("this is a cute value",
                 EqualsHeader(headers, "X-Random-Header"));
  }

  SECTION("use body as data")
  {
    REQUIRE_NOTHROW(response.content());
    auto content = response.content();
    auto json = nlohmann::json::parse(content.begin(), content.end());
    auto const& headers = json.at("headers");
    REQUIRE_THAT("this is a cute value",
                 EqualsHeader(headers, "X-Random-Header"));
  }
}

TEST_CASE_METHOD(ioc_fixture,
                 "async fetch basic auth",
                 "[https][fetch][basic_auth][authorization]")
{
  auto request =
      make_request(fetchpp::http::verb::get,
                   fetchpp::url::parse("basic-auth/david/totopaf"_https));
  request.set(fetchpp::http::authorization::basic("david", "totopaf"));
  auto response =
      fetchpp::async_fetch(ioc, std::move(request), boost::asio::use_future)
          .get();

  REQUIRE(response.result_int() == 200);
}

TEST_CASE_METHOD(ioc_fixture,
                 "async fetch bearer auth",
                 "[https][fetch][bearer_auth][authorization]")
{
  auto request = make_request(fetchpp::http::verb::get,
                              fetchpp::url::parse("bearer"_https));
  request.set(
      fetchpp::http::authorization::bearer("this_is_a_bearer_token_probably"));
  auto response =
      fetchpp::async_fetch(ioc, std::move(request), boost::asio::use_future)
          .get();

  REQUIRE(response.result_int() == 200);
}

TEST_CASE_METHOD(ioc_fixture,
                 "async fetch with ssl context",
                 "[https][fetch][get][async][verify_peer][!mayfail]")
{
  auto request =
      make_request(fetchpp::http::verb::get, fetchpp::url::parse("get"_https));
  request.set("X-Random-Header", "this is a cute value");

  using fetchpp::net::ssl::context;
  auto sslc = context{context::tls_client};
  // We need this for some broken build of boost ssl backend
  sslc.add_verify_path("/etc/ssl/certs");
  sslc.add_verify_path("/etc/ssl/private");
  sslc.set_verify_mode(context::verify_peer);
  auto response = fetchpp::async_fetch(
                      ioc, sslc, std::move(request), boost::asio::use_future)
                      .get();
  REQUIRE(response.result_int() == 200);
  REQUIRE(response.at(fetchpp::field::content_type) == "application/json");
  auto const json = response.json();
  auto const& headers = json.at("headers");
  REQUIRE_THAT("this is a cute value",
               EqualsHeader(headers, "X-Random-Header"));
}

TEST_CASE_METHOD(ioc_fixture, "http async get", "[https][get][async]")
{
  auto response =
      fetchpp::async_get(
          ioc,
          "get"_https,
          fetchpp::http::headers{{"X-random-header", "this is a cute value"}},
          boost::asio::use_future)
          .get();
  REQUIRE(response.result_int() == 200);
  REQUIRE(response.content_type().type() == "application/json");
  auto const json = response.json();
  auto ct = std::string{to_string(fetchpp::field::content_type)};
  REQUIRE_THAT("this is a cute value",
               EqualsHeader(json.at("headers"), "X-Random-Header"));
}

TEST_CASE_METHOD(ioc_fixture, "async post string", "[https][post][async]")
{
  auto const data = std::string("this is my data");
  auto response = fetchpp::async_post<StringRequest>(
                      ioc,
                      "post"_https,
                      std::move(data),
                      fetchpp::http::headers{{"X-corp-header", "corp value"}},
                      boost::asio::use_future)
                      .get();
  REQUIRE(response.result_int() == 200);
  REQUIRE(response.at(fetchpp::field::content_type) == "application/json");
  auto const& json = response.json();
  REQUIRE(json.at("headers").at("X-Corp-Header") == "corp value");
  REQUIRE(json.at("data") == data);
}

TEST_CASE_METHOD(ioc_fixture, "async post json", "[https][post][json][async]")
{
  auto response = fetchpp::async_post<JsonRequest>(
                      ioc,
                      "post"_https,
                      {{{"a key", "a value"}}},
                      fetchpp::http::headers{{"X-corp-header", "corp value"}},
                      boost::asio::use_future)
                      .get();
  REQUIRE(response.result_int() == 200);
  auto const& json = response.json();
  CHECK(json.at("headers").at("X-Corp-Header") == "corp value");
  CHECK(json.at("json") == nlohmann::json({{"a key", "a value"}}));
}
