#include <catch2/catch.hpp>

#include <fetchpp/core/connect.hpp>
#include <fetchpp/core/process_one.hpp>
#include <fetchpp/version.hpp>

#include <fetchpp/http/field.hpp>
#include <fetchpp/http/request.hpp>
#include <fetchpp/http/response.hpp>

#include <boost/asio/use_future.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/span_body.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>

#include "helpers/format.hpp"
#include "helpers/https_connect.hpp"
#include "helpers/ioc_fixture.hpp"
#include "helpers/test_domain.hpp"

#include <nlohmann/json.hpp>

using namespace test::helpers;
using namespace test::helpers::http_literals;
using test::helpers::ioc_fixture;

namespace fetchpp
{
namespace ssl = net::ssl;
}

TEST_CASE_METHOD(ioc_fixture,
                 "process one get simple_response",
                 "[https][process_one][async]")
{
  fetchpp::ssl::context context(fetchpp::ssl::context::tlsv12_client);
  fetchpp::beast::ssl_stream<fetchpp::beast::tcp_stream> stream(ioc, context);
  fetchpp::beast::flat_buffer buffer;
  auto const url = fetchpp::http::url::parse("/get"_https);
  fetchpp::http::request<fetchpp::http::empty_body> request(
      fetchpp::http::verb::get, url, fetchpp::options{});
  fetchpp::beast::http::response<fetchpp::http::string_body> response;

  http_ssl_connect(ioc, stream, url);

  auto fut = fetchpp::async_process_one(
      stream, buffer, request, response, boost::asio::use_future);
  fut.get();
  REQUIRE(response.result_int() == 200);
  REQUIRE(response.at(fetchpp::http::field::content_type) ==
          "application/json");
}

TEST_CASE_METHOD(ioc_fixture,
                 "process one get reponse",
                 "[https][process_one][async]")
{
  fetchpp::ssl::context context(fetchpp::ssl::context::tlsv12_client);
  fetchpp::beast::ssl_stream<fetchpp::beast::tcp_stream> stream(ioc, context);
  fetchpp::beast::flat_buffer buffer;
  auto const url = fetchpp::http::url::parse("/get"_https);
  fetchpp::http::request<fetchpp::http::empty_body> request(
      fetchpp::http::verb::get, url, fetchpp::options{});
  fetchpp::http::response response;

  http_ssl_connect(ioc, stream, url);

  auto fut = fetchpp::async_process_one(
      stream, buffer, request, response, boost::asio::use_future);
  fut.get();
  REQUIRE(response.result_int() == 200);
  REQUIRE(response.at(fetchpp::http::field::content_type) ==
          "application/json");
  REQUIRE(response.is_json());
  REQUIRE_NOTHROW(response.json());
}

TEST_CASE_METHOD(ioc_fixture, "connect", "[https][connect][async]")
{
  fetchpp::ssl::context context(fetchpp::ssl::context::tlsv12_client);
  fetchpp::beast::ssl_stream<fetchpp::beast::tcp_stream> stream(ioc, context);
  fetchpp::beast::flat_buffer buffer;

  auto const url = fetchpp::http::url::parse("/get"_https);
  auto request = fetchpp::http::make_request(
      fetchpp::http::verb::get, url, fetchpp::options{});
  request.set(fetchpp::http::field::host, std::string{get_test_host()});
  request.set(fetchpp::http::field::user_agent, fetchpp::USER_AGENT);
  fetchpp::http::response response;

  auto results = http_resolve_domain(ioc, url);
  auto fut = fetchpp::async_connect(
      stream, url.domain(), results, boost::asio::use_future);
  fut.get();

  auto fut2 = fetchpp::async_process_one(
      stream, buffer, request, response, boost::asio::use_future);
  fut2.get();
  REQUIRE(response.result_int() == 200);
  REQUIRE(response.at(fetchpp::http::field::content_type) ==
          "application/json");
}

TEST_CASE_METHOD(ioc_fixture,
                 "async_process_one body string",
                 "[https][process_one][async][body]")
{
  fetchpp::ssl::context context(fetchpp::ssl::context::tlsv12_client);
  fetchpp::beast::ssl_stream<fetchpp::beast::tcp_stream> stream(ioc, context);
  fetchpp::beast::flat_buffer buffer;

  auto data = std::string("my dearest data");
  auto const url = fetchpp::http::url::parse("/anything"_https);
  auto request = fetchpp::http::make_request<
      fetchpp::http::request<fetchpp::http::span_body<char>>>(
      fetchpp::http::verb::post, url, {}, boost::beast::span<char>(data));
  request.set(fetchpp::http::field::content_type, "text/plain");
  fetchpp::http::response response;

  auto results = http_resolve_domain(ioc, url);
  auto fut = fetchpp::async_connect(
      stream, url.domain(), results, boost::asio::use_future);
  fut.get();

  auto fut2 = fetchpp::async_process_one(
      stream, buffer, request, response, boost::asio::use_future);
  fut2.get();
  REQUIRE(response.result_int() == 200);
  REQUIRE(response.content_type().type() == "application/json");
  REQUIRE(nlohmann::json::parse(response.text()).at("data") == data);
}
