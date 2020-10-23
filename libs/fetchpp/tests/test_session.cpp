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

#include <array>
#include <string>

#include <catch2/catch.hpp>

using namespace test::helpers::http_literals;
using namespace std::chrono_literals;

using test::helpers::HasErrorCode;
using test::helpers::ioc_fixture;

using AsyncStream = fetchpp::beast::ssl_stream<fetchpp::beast::tcp_stream>;
namespace ssl = fetchpp::net::ssl;
using boost::asio::use_future;
using URL = fetchpp::http::url;

TEST_CASE_METHOD(ioc_fixture,
                 "session fails to auto connect on bad domain",
                 "[session][connect]")
{
  ssl::context context(ssl::context::tlsv12_client);
  auto url = fetchpp::http::url("https://127.0.0.2:444");
  auto session = fetchpp::session(
      fetchpp::secure_endpoint("127.0.0.2", 444), 3s, ioc, context);
  auto request = fetchpp::http::request(fetchpp::http::verb::get, url);
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
      fetchpp::detail::to_endpoint<true>(url), 30s, ioc, context);

  auto request = fetchpp::http::request(fetchpp::http::verb::get,
                                        fetchpp::http::url("get"_https));
  {
    session.async_start(use_future).get();
    fetchpp::http::response response;
    session.push_request(request, response, use_future).get();
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
                                  ioc,
                                  context);
  auto request = fetchpp::http::request(fetchpp::http::verb::get,
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
                 "[session][delay]")
{
  ssl::context sslc(ssl::context::tlsv12_client);
  auto const url = fetchpp::http::url("delay/4"_http);
  auto request = fetchpp::http::request(fetchpp::http::verb::get, url);
  auto session = fetchpp::session(
      fetchpp::secure_endpoint("10.255.255.1", 444), 3s, ioc, sslc);
  fetchpp::http::response response;
  REQUIRE_THROWS_MATCHES(
      session.push_request(request, response, fetchpp::net::use_future).get(),
      boost::system::system_error,
      HasErrorCode(boost::beast::error::timeout));
}

TEST_CASE_METHOD(ioc_fixture,
                 "session aborts a request that takes to much time",
                 "[session][delay]")
{
  auto const url = fetchpp::http::url("delay/4"_http);
  auto session =
      fetchpp::session(fetchpp::detail::to_endpoint<false>(url), 2s, ex);
  auto request = fetchpp::http::request(fetchpp::http::verb::get, url);
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
  auto session =
      fetchpp::session(fetchpp::detail::to_endpoint(url), 30s, ioc, context);
  auto request = fetchpp::http::request(fetchpp::http::verb::get, url);
  auto request2 = fetchpp::http::request(fetchpp::http::verb::get, url);

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
  auto const urlget = fetchpp::http::url("get"_http);
  auto const urldelay = fetchpp::http::url("delay/3"_http);
  auto session =
      fetchpp::session(fetchpp::detail::to_endpoint<false>(urlget), 2s, ex);

  auto get = fetchpp::http::request(fetchpp::http::verb::get, urlget);
  auto delay = fetchpp::http::request(fetchpp::http::verb::get, urldelay);

  fetchpp::http::response response;
  auto fut = session.push_request(get, response, fetchpp::net::use_future);

  fetchpp::http::response delay_response;
  auto delay_fut =
      session.push_request(delay, delay_response, fetchpp::net::use_future);

  fut.get();
  REQUIRE(response.result_int() == 200);

  REQUIRE_THROWS_MATCHES(
      delay_fut.get(),
      boost::system::system_error,
      HasErrorCode(boost::beast::error::timeout) ||
          HasErrorCode(boost::asio::ssl::error::stream_truncated));
  REQUIRE(session.pending_requests() == 0);
}

TEST_CASE_METHOD(ioc_fixture,
                 "session closes while connecting",
                 "[session][https][close][delay]")
{
  auto context = ssl::context(ssl::context::tlsv12_client);
  auto const urldelay = fetchpp::http::url("delay/1"_https);
  auto session = fetchpp::session(
      fetchpp::detail::to_endpoint<true>(urldelay), 5s, ex, context);

  {
    auto delay = fetchpp::http::request(fetchpp::http::verb::get, urldelay);
    std::vector<fetchpp::http::response> responses;
    std::vector<std::future<void>> futures;
    for (int i = 1; i < 10; ++i)
      futures.push_back(session.push_request(
          delay, responses.emplace_back(), boost::asio::use_future));

    REQUIRE_NOTHROW(session.async_stop(boost::asio::use_future).get());
    for (auto& future : futures)
      future.wait();
    REQUIRE(session.pending_requests() == 0);
  }
}

TEST_CASE_METHOD(ioc_fixture,
                 "session closes while multiple request are executed",
                 "[session][https][close][delay]")
{
  auto context = ssl::context(ssl::context::tlsv12_client);
  auto const urldelay = fetchpp::http::url("delay/1"_https);
  auto session = fetchpp::session(
      fetchpp::detail::to_endpoint<true>(urldelay), 5s, ex, context);

  // wait for the connecton to established
  {
    auto const urlget = fetchpp::http::url("get"_https);
    auto get = fetchpp::http::request(fetchpp::http::verb::get, urlget);
    fetchpp::http::response response;
    auto fut = session.push_request(get, response, fetchpp::net::use_future);
    REQUIRE_NOTHROW(fut.get());
    REQUIRE(response.result_int() == 200);
  }

  {
    auto delay = fetchpp::http::request(fetchpp::http::verb::get, urldelay);
    std::vector<fetchpp::http::response> responses(10);
    std::vector<std::future<void>> futures;
    for (int i = 1; i < 10; ++i)
      futures.push_back(
          session.push_request(delay, responses[i], boost::asio::use_future));

    REQUIRE_NOTHROW(session.async_stop(boost::asio::use_future).get());
    for (auto& future : futures)
      future.wait();
    REQUIRE(session.pending_requests() == 0);
  }
}

TEST_CASE_METHOD(ioc_fixture,
                 "session executes one request through proxy tunnel",
                 "[session][proxy][push]")
{
  auto const url = URL("get"_https);
  ssl::context ctx(ssl::context::tlsv12_client);
  auto const proxy_url = URL(test::helpers::get_test_proxy());
  auto const tunnel =
      fetchpp::tunnel_endpoint{fetchpp::detail::to_endpoint<false>(proxy_url),
                               fetchpp::detail::to_endpoint<true>(url)};
  auto session = fetchpp::session(tunnel, 30s, ex, ctx);

  auto request = fetchpp::http::request(fetchpp::http::verb::get,
                                        fetchpp::http::url("get"_https));
  {
    session.async_start(use_future).get();
    fetchpp::http::response response;
    session.push_request(request, response, use_future).get();
    REQUIRE(response.result_int() == 200);
  }
}
