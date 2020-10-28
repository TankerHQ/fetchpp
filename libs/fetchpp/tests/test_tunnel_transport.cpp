#include <fetchpp/core/tunnel_transport.hpp>

#include <fetchpp/http/request.hpp>
#include <fetchpp/http/response.hpp>

#include "helpers/format.hpp"
#include "helpers/ioc_fixture.hpp"
#include "helpers/match_exception.hpp"
#include "helpers/test_domain.hpp"

#include <fetchpp/core/detail/endpoint.hpp>

#include <catch2/catch.hpp>

#include <boost/asio/use_future.hpp>

#include <chrono>

using namespace std::chrono_literals;
namespace ssl = fetchpp::net::ssl;
using boost::asio::use_future;
using namespace test::helpers::http_literals;

using test::helpers::ioc_fixture;
using URL = fetchpp::http::url;
using test::helpers::HasErrorCode;

TEST_CASE_METHOD(ioc_fixture, "connect timeouts", "[tunnel][http][timeout]")
{
  ssl::context ctx(ssl::context::tlsv12_client);
  fetchpp::tunnel_async_transport ts(500ms, ex, ctx);
  // this is a non routable ip
  auto proxy = fetchpp::plain_endpoint("10.255.255.1", 8080);
  auto const now = std::chrono::high_resolution_clock::now();
  auto const url = URL("get"_https);
  REQUIRE_THROWS_MATCHES(
      ts.async_connect({proxy, fetchpp::detail::to_endpoint<true>(url)},
                       boost::asio::use_future)
          .get(),
      boost::system::system_error,
      HasErrorCode(boost::beast::error::timeout));
  auto const end = std::chrono::high_resolution_clock::now();
  REQUIRE((650ms) > (end - now));
  REQUIRE_NOTHROW(ts.async_close(boost::asio::use_future).get());
}

TEST_CASE_METHOD(ioc_fixture, "transport one ssl", "[tunnel][https]")
{
  ssl::context ctx(ssl::context::tlsv12_client);
  auto const url = URL("get"_https);
  auto const proxy_url = URL(test::helpers::get_test_proxy());
  auto const tunnel =
      fetchpp::tunnel_endpoint{fetchpp::detail::to_endpoint<false>(proxy_url),
                               fetchpp::detail::to_endpoint<true>(url)};

  fetchpp::tunnel_async_transport ts(5s, ex, ctx);
  REQUIRE_NOTHROW(ts.async_connect(tunnel, boost::asio::use_future).get());
  auto const request = fetchpp::http::request(fetchpp::http::verb::get, url);
  fetchpp::http::response response;
  REQUIRE_NOTHROW(
      fetchpp::async_process_one(ts, request, response, boost::asio::use_future)
          .get());
  CHECK(response.result_int() == 200);
  REQUIRE_NOTHROW(ts.async_close(boost::asio::use_future).get());
}

TEST_CASE_METHOD(ioc_fixture,
                 "transport one ssl over timeout",
                 "[tunnel][https][delay]")
{
  auto const url = URL("delay/5"_https);
  ssl::context ctx(ssl::context::tlsv12_client);
  auto const proxy_url = URL(test::helpers::get_test_proxy());
  auto const tunnel =
      fetchpp::tunnel_endpoint{fetchpp::detail::to_endpoint<false>(proxy_url),
                               fetchpp::detail::to_endpoint<true>(url)};
  fetchpp::tunnel_async_transport ts(1s, ex, ctx);
  REQUIRE_NOTHROW(ts.async_connect(tunnel, boost::asio::use_future).get());
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
                 "[tunnel][https][close]")
{
  auto const url = URL("get"_https);
  ssl::context ctx(ssl::context::tlsv12_client);
  auto const proxy_url = URL(test::helpers::get_test_proxy());
  auto const tunnel =
      fetchpp::tunnel_endpoint{fetchpp::detail::to_endpoint<false>(proxy_url),
                               fetchpp::detail::to_endpoint<true>(url)};

  fetchpp::tunnel_async_transport ts(5s, ex, ctx);
  REQUIRE_NOTHROW(ts.async_connect(tunnel, boost::asio::use_future).get());
  auto const request = fetchpp::http::request(fetchpp::http::verb::get, url);
  fetchpp::http::response response;
  REQUIRE_NOTHROW(
      fetchpp::async_process_one(ts, request, response, boost::asio::use_future)
          .get());
  REQUIRE_NOTHROW(ts.async_close(boost::asio::use_future).get());
  REQUIRE_NOTHROW(ts.async_connect(tunnel, boost::asio::use_future).get());
}
