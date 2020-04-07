#include <catch2/catch.hpp>

#include <boost/beast/core/buffers_to_string.hpp>

#include <fetchpp/fetch.hpp>
#include <fetchpp/field.hpp>
#include <fetchpp/json_body.hpp>
#include <fetchpp/message.hpp>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/use_future.hpp>

#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>

#include "helpers/format.hpp"
#include "helpers/ioc_fixture.hpp"
#include "helpers/test_domain.hpp"

#include <fmt/format.h>
#include <nlohmann/json.hpp>

using namespace test::helpers::http_literals;
using test::helpers::ioc_fixture;

TEST_CASE_METHOD(ioc_fixture, "async fetch", "[https][fetch][get][async]")
{
  auto request =
      make_request(fetchpp::http::verb::get, fetchpp::url::parse("get"_https));
  request.content_type("text/html; charset=UTF8");
  auto response = fetchpp::async_fetch<fetchpp::http::string_body>(
                      ioc, std::move(request), boost::asio::use_future)
                      .get();
  REQUIRE(response.result_int() == 200);
  REQUIRE(response.at(fetchpp::field::content_type) == "application/json");
  auto json = nlohmann::json::parse(response.body());
  auto const content_type = json.at("headers").at(
      std::string{to_string(fetchpp::field::content_type)});
  REQUIRE(content_type == "text/html; charset=UTF8");
}

TEST_CASE_METHOD(ioc_fixture, "http async get", "[https][get][async]")
{
  auto response = fetchpp::async_get<fetchpp::http::string_body>(
                      ioc,
                      "get"_https,
                      fetchpp::headers{{fetchpp::field::content_type,
                                        "text/html; charset=UTF8"}},
                      boost::asio::use_future)
                      .get();
  REQUIRE(response.result_int() == 200);
  REQUIRE(response.at(fetchpp::field::content_type) == "application/json");
  auto const& body = response.body();
  auto json = nlohmann::json::parse(body.begin(), body.end());
  auto ct = std::string{to_string(fetchpp::field::content_type)};
  REQUIRE(json.at("headers").at(ct) == "text/html; charset=UTF8");
}

TEST_CASE_METHOD(ioc_fixture, "async post string", "[https][post][async]")
{
  auto const data = std::string("this is my data");
  auto response = fetchpp::async_post<fetchpp::http::string_body,
                                      fetchpp::http::string_body>(
                      ioc,
                      "post"_https,
                      std::move(data),
                      fetchpp::headers{{"X-corp-header", "corp value"}},
                      boost::asio::use_future)
                      .get();
  REQUIRE(response.result_int() == 200);
  REQUIRE(response.at(fetchpp::field::content_type) == "application/json");
  auto const& body = response.body();
  auto json = nlohmann::json::parse(body.begin(), body.end());
  REQUIRE(json.at("headers").at("X-Corp-Header") == "corp value");
  REQUIRE(json.at("data") == data);
}

TEST_CASE_METHOD(ioc_fixture, "async post json", "[https][post][json][async]")
{
  auto response = fetchpp::async_post<fetchpp::json_body, fetchpp::json_body>(
                      ioc,
                      "post"_https,
                      {{{"a key", "a value"}}},
                      fetchpp::headers{{"X-corp-header", "corp value"}},
                      boost::asio::use_future)
                      .get();
  REQUIRE(response.body().at("headers").at("X-Corp-Header") == "corp value");
  REQUIRE(response.body().at("json") == nlohmann::json({{"a key", "a value"}}));
}
