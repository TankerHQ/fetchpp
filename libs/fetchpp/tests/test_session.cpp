// #include <fetchpp/client.hpp>
#include <fmt/format.h>

#include <fetchpp/core/ssl_transport.hpp>

#include <fetchpp/core/session.hpp>
#include <fetchpp/http/json_body.hpp>
#include <fetchpp/http/request.hpp>
#include <fetchpp/http/response.hpp>

#include "helpers/format.hpp"
#include "helpers/ioc_fixture.hpp"
#include "helpers/match_exception.hpp"
#include "helpers/test_domain.hpp"

#include <boost/asio/ssl/context.hpp>
#include <boost/asio/use_future.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>

#include <fetchpp/alias/net.hpp>

#include <fmt/ostream.h>

#include <string>

#include <catch2/catch.hpp>

using namespace test::helpers::http_literals;
using test::helpers::HasErrorCode;
using test::helpers::ioc_fixture;

using AsyncStream = fetchpp::beast::ssl_stream<fetchpp::beast::tcp_stream>;
namespace ssl = fetchpp::net::ssl;

TEST_CASE_METHOD(ioc_fixture,
                 "session fails to auto connect on bad domain",
                 "[session][connect]")
{
  ssl::context context(ssl::context::tlsv12_client);
  auto url = fetchpp::http::url::parse("https://127.0.0.2:444");
  auto session =
      fetchpp::session(fetchpp::secure_endpoint(url.domain(), url.port()),
                       AsyncStream(ioc, context));
  auto request = fetchpp::http::make_request(fetchpp::http::verb::get, url);
  fetchpp::http::response response;
  REQUIRE_THROWS_MATCHES(
      session.push_request(request, response, boost::asio::use_future).get(),
      boost::system::system_error,
      HasErrorCode(boost::asio::error::connection_refused) ||
          HasErrorCode(boost::asio::error::timed_out));
}

TEST_CASE_METHOD(
    ioc_fixture,
    "session executes one request pushed after explicit connection",
    "[session][push]")
{
  ssl::context context(ssl::context::tlsv12_client);
  auto const url = fetchpp::http::url::parse("get"_https);
  auto session =
      fetchpp::session(fetchpp::secure_endpoint(url.domain(), url.port()),
                       AsyncStream(ioc, context));
  REQUIRE_NOTHROW(session.async_start(boost::asio::use_future).get());

  auto request = fetchpp::http::make_request(
      fetchpp::http::verb::get, fetchpp::http::url::parse("get"_https));
  {
    fetchpp::http::response response;
    session.push_request(request, response, boost::asio::use_future).get();
    REQUIRE(response.result_int() == 200);
  }
}

TEST_CASE_METHOD(ioc_fixture,
                 "session executes two requests pushed",
                 "[session][push]")
{
  ssl::context context(ssl::context::tlsv12_client);
  auto const url = fetchpp::http::url::parse("get"_https);
  auto session =
      fetchpp::session(fetchpp::secure_endpoint(url.domain(), url.port()),
                       AsyncStream(ioc, context));
  auto request = fetchpp::http::make_request(
      fetchpp::http::verb::get, fetchpp::http::url::parse("get"_https));
  {
    fetchpp::http::response response;
    session.push_request(request, response, boost::asio::use_future).get();
    REQUIRE(response.result_int() == 200);
  }
  {
    fetchpp::beast::http::response<fetchpp::http::json_body> response;
    session.push_request(request, response, boost::asio::use_future).get();
    REQUIRE(response.result_int() == 200);
  }
  auto fut2 = session.async_stop(boost::asio::use_future);
  REQUIRE_NOTHROW(fut2.get());
}

TEST_CASE("session executes multiple requests pushed", "[session][push]")
{
  fetchpp::net::io_context ioc;
  ssl::context context(ssl::context::tlsv12_client);
  auto const url = fetchpp::http::url::parse("get"_https);
  auto session =
      fetchpp::session(fetchpp::secure_endpoint(url.domain(), url.port()),
                       AsyncStream(ioc, context));
  auto request = fetchpp::http::make_request(
      fetchpp::http::verb::get, fetchpp::http::url::parse("get"_https));
  auto request2 = fetchpp::http::make_request<
      fetchpp::http::request<fetchpp::http::json_body>>(
      fetchpp::http::verb::get, fetchpp::http::url::parse("get"_https));

  fetchpp::http::response response;
  auto fut = session.push_request(request, response, fetchpp::net::use_future);

  fetchpp::http::response response2;
  auto fut2 =
      session.push_request(request2, response2, fetchpp::net::use_future);

  ioc.run(); // run all the requests.

  fut.get();
  REQUIRE(response.result_int() == 200);

  fut2.get();
  REQUIRE(response2.result_int() == 200);
}
