#include <fetchpp/core/process_one.hpp>
#include <fetchpp/core/ssl_transport.hpp>
#include <fetchpp/core/tcp_transport.hpp>
#include <fetchpp/http/request.hpp>
#include <fetchpp/http/response.hpp>

#include <fetchpp/core/detail/endpoint.hpp>

#include <boost/asio/ssl/context.hpp>
#include <boost/asio/use_future.hpp>

#include "helpers/format.hpp"
#include "helpers/ioc_fixture.hpp"
#include "helpers/match_exception.hpp"
#include "helpers/test_domain.hpp"

#include <catch2/catch.hpp>

#include <fmt/format.h>

#include <chrono>

using namespace std::chrono_literals;

namespace ssl = fetchpp::net::ssl;
using namespace test::helpers::http_literals;

using test::helpers::HasErrorCode;
using test::helpers::ioc_fixture;
using URL = fetchpp::http::url;

TEST_CASE_METHOD(ioc_fixture, "connect timeouts", "[transport][http][timeout]")
{
  fetchpp::tcp_async_transport ts(500ms, ioc);
  // this is a non routable ip
  auto endpoint = fetchpp::plain_endpoint("10.255.255.1", 8080);
  auto const now = std::chrono::high_resolution_clock::now();
  REQUIRE_THROWS_MATCHES(
      ts.async_connect(endpoint, boost::asio::use_future).get(),
      boost::system::system_error,
      HasErrorCode(boost::beast::error::timeout));
  auto const end = std::chrono::high_resolution_clock::now();
  REQUIRE((650ms) > (end - now));
  REQUIRE_NOTHROW(ts.async_close(boost::asio::use_future).get());
}

TEST_CASE_METHOD(ioc_fixture, "transport one ssl", "[transport][https]")
{
  auto const url = URL("get"_https);
  ssl::context context(ssl::context::tlsv12_client);

  fetchpp::ssl_async_transport ts(5s, ioc, context);
  auto endpoint = fetchpp::detail::to_endpoint<true>(url);
  REQUIRE_NOTHROW(ts.async_connect(endpoint, boost::asio::use_future).get());
  auto const request = fetchpp::http::request(fetchpp::http::verb::get, url);
  fetchpp::http::response response;
  REQUIRE_NOTHROW(
      fetchpp::async_process_one(ts, request, response, boost::asio::use_future)
          .get());
  CHECK(response.result_int() == 200);
  REQUIRE_NOTHROW(ts.async_close(boost::asio::use_future).get());
}

TEST_CASE_METHOD(ioc_fixture, "transport one tcp", "[transport][http]")
{
  auto const url = URL("get"_http);

  fetchpp::tcp_async_transport ts(5s, ioc);
  auto endpoint = fetchpp::detail::to_endpoint<false>(url);
  REQUIRE_NOTHROW(ts.async_connect(endpoint, boost::asio::use_future).get());
  auto const request = fetchpp::http::request(fetchpp::http::verb::get, url);
  fetchpp::http::response response;
  REQUIRE_NOTHROW(
      fetchpp::async_process_one(ts, request, response, boost::asio::use_future)
          .get());
  CHECK(response.result_int() == 200);
  REQUIRE_NOTHROW(ts.async_close(boost::asio::use_future).get());
}

TEST_CASE_METHOD(ioc_fixture,
                 "transport one tcp over timeout",
                 "[transport][http][delay]")
{
  auto const url = URL("delay/5"_http);

  fetchpp::tcp_async_transport ts(1s, ioc);
  auto endpoint = fetchpp::detail::to_endpoint<false>(url);
  REQUIRE_NOTHROW(ts.async_connect(endpoint, boost::asio::use_future).get());
  auto const request = fetchpp::http::request(fetchpp::http::verb::get, url);
  fetchpp::http::response response;
  auto const now = std::chrono::high_resolution_clock::now();
  REQUIRE_THROWS_MATCHES(
      fetchpp::async_process_one(ts, request, response, boost::asio::use_future)
          .get(),
      boost::system::system_error,
      HasErrorCode(boost::beast::error::timeout));
  auto const end = std::chrono::high_resolution_clock::now();
  REQUIRE(1.5s > end - now);
  REQUIRE_NOTHROW(ts.async_close(boost::asio::use_future).get());
}

TEST_CASE_METHOD(ioc_fixture,
                 "transport one ssl over timeout",
                 "[transport][https][delay]")
{
  auto const url = URL("delay/5"_https);
  ssl::context context(ssl::context::tlsv12_client);

  fetchpp::ssl_async_transport ts(1s, ioc, context);
  auto endpoint = fetchpp::detail::to_endpoint<true>(url);
  REQUIRE_NOTHROW(ts.async_connect(endpoint, boost::asio::use_future).get());
  auto const request = fetchpp::http::request(fetchpp::http::verb::get, url);
  fetchpp::http::response response;
  auto const now = std::chrono::high_resolution_clock::now();
  REQUIRE_THROWS_MATCHES(
      fetchpp::async_process_one(ts, request, response, boost::asio::use_future)
          .get(),
      boost::system::system_error,
      HasErrorCode(boost::beast::error::timeout));
  auto const end = std::chrono::high_resolution_clock::now();
  REQUIRE(1.5s > end - now);
}

TEST_CASE_METHOD(ioc_fixture,
                 "transport closes and reopens",
                 "[transport][https][close]")
{
  auto const url = URL("get"_https);
  ssl::context context(ssl::context::tlsv12_client);

  fetchpp::ssl_async_transport ts(5s, ioc, context);
  auto endpoint = fetchpp::detail::to_endpoint<true>(url);
  REQUIRE_NOTHROW(ts.async_connect(endpoint, boost::asio::use_future).get());
  auto const request = fetchpp::http::request(fetchpp::http::verb::get, url);
  fetchpp::http::response response;
  REQUIRE_NOTHROW(
      fetchpp::async_process_one(ts, request, response, boost::asio::use_future)
          .get());
  REQUIRE_NOTHROW(ts.async_close(boost::asio::use_future).get());
  REQUIRE_NOTHROW(ts.async_connect(endpoint, boost::asio::use_future).get());
}
