// #include <fetchpp/client.hpp>
#include <fmt/format.h>

#include <fetchpp/core/session.hpp>

#include <fetchpp/core/ssl_transport.hpp>
#include <fetchpp/http/json_body.hpp>
#include <fetchpp/http/request.hpp>
#include <fetchpp/http/response.hpp>

#include <fetchpp/core/detail/endpoint.hpp>

#include "helpers/format.hpp"
#include "helpers/ioc_fixture.hpp"
#include "helpers/match_exception.hpp"
#include "helpers/test_domain.hpp"

#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/use_future.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>

#include <fetchpp/alias/net.hpp>

#include <fmt/ostream.h>

#include <string>

#include <catch2/catch.hpp>

using namespace test::helpers::http_literals;
using namespace std::chrono_literals;

using test::helpers::HasErrorCode;
using test::helpers::ioc_fixture;

using AsyncStream = fetchpp::beast::ssl_stream<fetchpp::beast::tcp_stream>;
namespace ssl = fetchpp::net::ssl;

TEST_CASE_METHOD(ioc_fixture,
                 "session fails to auto connect on bad domain",
                 "[session][connect]")
{
  ssl::context context(ssl::context::tlsv12_client);
  auto url = fetchpp::http::url("https://127.0.0.2:444");
  auto session = fetchpp::session(fetchpp::secure_endpoint("127.0.0.2", 444),
                                  3s,
                                  AsyncStream(ioc, context));
  auto request = fetchpp::http::make_request(fetchpp::http::verb::get, url);
  fetchpp::http::response response;
  REQUIRE_THROWS_MATCHES(
      session.push_request(request, response, boost::asio::use_future).get(),
      boost::system::system_error,
      HasErrorCode(boost::asio::error::connection_refused) ||
          HasErrorCode(boost::asio::error::timed_out) ||
          HasErrorCode(boost::beast::error::timeout));
}

TEST_CASE_METHOD(
    ioc_fixture,
    "session executes one request pushed after explicit connection",
    "[session][push]")
{
  ssl::context context(ssl::context::tlsv12_client);
  auto const url = fetchpp::http::url("get"_https);
  auto session = fetchpp::session(
      fetchpp::detail::to_endpoint<true>(url), 30s, AsyncStream(ioc, context));
  REQUIRE_NOTHROW(session.async_start(boost::asio::use_future).get());

  auto request = fetchpp::http::make_request(fetchpp::http::verb::get,
                                             fetchpp::http::url("get"_https));
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
  auto const url = fetchpp::http::url("get"_https);
  auto session = fetchpp::session(fetchpp::detail::to_endpoint(url),
                                  std::chrono::seconds(30),
                                  AsyncStream(ioc, context));
  auto request = fetchpp::http::make_request(fetchpp::http::verb::get,
                                             fetchpp::http::url("get"_https));
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

TEST_CASE_METHOD(ioc_fixture,
                 "sessions abort when connection times out",
                 "[session][drip]")
{
  ssl::context sslc(ssl::context::tlsv12_client);
  auto session = fetchpp::session(fetchpp::secure_endpoint("10.255.255.1", 444),
                                  3s,
                                  AsyncStream(ioc, sslc));
  REQUIRE_THROWS_MATCHES(session.async_start(boost::asio::use_future).get(),
                         boost::system::system_error,
                         HasErrorCode(boost::beast::error::timeout));
}

TEST_CASE_METHOD(ioc_fixture,
                 "session aborts a request that takes to much time",
                 "[session][delay]")
{
  auto const url = fetchpp::http::url("delay/4"_https);
  ssl::context sslc(ssl::context::tlsv12_client);
  auto session = fetchpp::session(
      fetchpp::detail::to_endpoint(url), 2s, AsyncStream(ioc, sslc));
  REQUIRE_NOTHROW(session.async_start(boost::asio::use_future).get());
  auto request = fetchpp::http::make_request(fetchpp::http::verb::get, url);
  fetchpp::http::response response;
  REQUIRE_THROWS_MATCHES(
      session.push_request(request, response, fetchpp::net::use_future).get(),
      boost::system::system_error,
      HasErrorCode(boost::beast::error::timeout));
}

TEST_CASE("session executes multiple requests pushed", "[session][push]")
{
  fetchpp::net::io_context ioc;
  ssl::context context(ssl::context::tlsv12_client);
  auto const url = fetchpp::http::url("get"_https);
  auto session = fetchpp::session(
      fetchpp::detail::to_endpoint(url), 30s, AsyncStream(ioc, context));
  auto request = fetchpp::http::make_request(fetchpp::http::verb::get, url);
  auto request2 = fetchpp::http::make_request<
      fetchpp::http::request<fetchpp::http::json_body>>(
      fetchpp::http::verb::get, url);

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

TEST_CASE_METHOD(ioc_fixture,
                 "session executes multiple requests pushed with some delay",
                 "[session][push][delay]")
{
  ssl::context context(ssl::context::tlsv12_client);
  auto const urlget = fetchpp::http::url("get"_https);
  auto const urldelay = fetchpp::http::url("delay/2"_https);
  auto session = fetchpp::session(
      fetchpp::detail::to_endpoint(urlget), 2s, AsyncStream(ex, context));
  auto get = fetchpp::http::make_request(fetchpp::http::verb::get, urlget);
  auto delay = fetchpp::http::make_request<
      fetchpp::http::request<fetchpp::http::json_body>>(
      fetchpp::http::verb::get, urldelay);

  fetchpp::http::response response;
  auto fut = session.push_request(get, response, fetchpp::net::use_future);

  fetchpp::http::response response2;
  auto fut2 = session.push_request(delay, response2, fetchpp::net::use_future);

  fut.get();
  REQUIRE(response.result_int() == 200);

  REQUIRE_THROWS_MATCHES(
      fut2.get(),
      boost::system::system_error,
      HasErrorCode(boost::beast::error::timeout) ||
          HasErrorCode(boost::asio::ssl::error::stream_truncated));
}