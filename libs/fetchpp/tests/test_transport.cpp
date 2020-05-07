#include <fetchpp/core/process_one.hpp>
#include <fetchpp/core/ssl_transport.hpp>
#include <fetchpp/core/tcp_transport.hpp>
#include <fetchpp/http/request.hpp>
#include <fetchpp/http/response.hpp>

#include <boost/asio/ssl/context.hpp>
#include <boost/asio/use_future.hpp>

#include "helpers/format.hpp"
#include "helpers/ioc_fixture.hpp"
#include "helpers/test_domain.hpp"

#include <catch2/catch.hpp>

namespace ssl = fetchpp::net::ssl;
using namespace test::helpers::http_literals;
using test::helpers::ioc_fixture;

TEST_CASE_METHOD(ioc_fixture, "transport one ssl", "[transport][https]")
{
  auto const url = fetchpp::http::url::parse("get"_https);
  ssl::context context(ssl::context::tlsv12_client);

  fetchpp::ssl_async_transport ts(url.domain(), url.port(), ioc, context);
  REQUIRE_NOTHROW(async_connect(ts, boost::asio::use_future).get());
  auto const request =
      fetchpp::http::make_request(fetchpp::http::verb::get, url);
  fetchpp::http::response response;
  REQUIRE_NOTHROW(
      fetchpp::async_process_one(ts, request, response, boost::asio::use_future)
          .get());
  CHECK(response.result_int() == 200);
  REQUIRE_NOTHROW(async_close(ts, boost::asio::use_future).get());
}

TEST_CASE_METHOD(ioc_fixture, "transport one tcp", "[transport][http]")
{
  auto const url = fetchpp::http::url::parse("get"_http);

  fetchpp::tcp_async_transport ts(url.domain(), url.port(), ioc);
  REQUIRE_NOTHROW(async_connect(ts, boost::asio::use_future).get());
  auto const request =
      fetchpp::http::make_request(fetchpp::http::verb::get, url);
  fetchpp::http::response response;
  REQUIRE_NOTHROW(
      fetchpp::async_process_one(ts, request, response, boost::asio::use_future)
          .get());
  CHECK(response.result_int() == 200);
  REQUIRE_NOTHROW(async_close(ts, boost::asio::use_future).get());
}
