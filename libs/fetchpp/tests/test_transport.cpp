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

#include "helpers/fake_server.hpp"
#include "helpers/https_connect.hpp"
#include "helpers/worker_fixture.hpp"

#include <fmt/chrono.h>
#include <fmt/format.h>

#include <chrono>

using namespace std::chrono_literals;

namespace ssl = fetchpp::net::ssl;
using namespace test::helpers::http_literals;

using test::helpers::HasErrorCode;
using test::helpers::ioc_fixture;
using test::helpers::worker_fixture;
using URL = fetchpp::http::url;

namespace net = boost::asio;
namespace bb = boost::beast;

namespace
{
auto tcp_endpoint_to_url(net::ip::tcp::endpoint const& endpoint)
{
  auto url = URL();
  url.set_host(endpoint.address().to_string());
  url.set_port(endpoint.port());
  return url;
}
}

namespace Catch
{
template <typename Rep, typename Period>
struct StringMaker<std::chrono::duration<Rep, Period>>
{
  static std::string convert(std::chrono::duration<Rep, Period> const& t)
  {
    return fmt::format("{}", t);
  }
};
}

TEST_CASE_METHOD(ioc_fixture, "connect timeouts", "[transport][http][timeout]")
{
  auto const transport_timout = GENERATE(100ms, 300ms);
  fetchpp::tcp_async_transport ts(ioc.get_executor(), transport_timout);
  // this is a non routable ip
  auto endpoint = fetchpp::plain_endpoint("10.255.255.1", 8080);
  auto const now = std::chrono::high_resolution_clock::now();
  CHECK_THROWS_MATCHES(
      ts.async_connect(endpoint, boost::asio::use_future).get(),
      boost::system::system_error,
      HasErrorCode(boost::beast::error::timeout));
  auto const end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - now);
  CHECK_NOFAIL(duration <= (transport_timout + transport_timout / 2));
  CHECK_NOTHROW(ts.async_close(boost::asio::use_future).get());
}

TEST_CASE_METHOD(ioc_fixture, "transport one ssl", "[transport][https]")
{
  auto const url = URL("get"_https);
  ssl::context context(ssl::context::tlsv12_client);

  fetchpp::ssl_async_transport ts(ioc.get_executor(), 5s, context);
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

TEST_CASE_METHOD(ioc_fixture,
                 "transport one tcp",
                 "[http][poly_body][transport]")
{
  auto const url = URL("get"_http);

  fetchpp::tcp_async_transport ts(ioc.get_executor(), 5s);
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

TEST_CASE_METHOD(worker_fixture,
                 "transport one fake tcp",
                 "[fake][http][poly_body][transport]")
{
  test::helpers::fake_server server(worker(1).ex);
  fetchpp::tcp_async_transport ts(worker().ex, 5s);

  auto url = tcp_endpoint_to_url(server.local_endpoint());
  url.set_pathname("/get");

  auto endpoint = fetchpp::detail::to_endpoint<false>(url);
  auto future = ts.async_connect(endpoint, boost::asio::use_future);
  auto fake_session = server.async_accept(fetchpp::net::use_future).get();
  REQUIRE_NOTHROW(future.get());

  auto const request = fetchpp::http::request(fetchpp::http::verb::get, url);
  fetchpp::http::response response;
  auto process_fut = fetchpp::async_process_one(
      ts, request, response, boost::asio::use_future);
  REQUIRE_NOTHROW(
      fake_session.async_reply_back(bb::http::status::ok, net::use_future)
          .get());
  CHECK(response.result_int() == 200);
  REQUIRE_NOTHROW(ts.async_close(boost::asio::use_future).get());
}

TEST_CASE_METHOD(worker_fixture,
                 "interrupt transport after server has received request",
                 "[fake][http][poly_body][transport]")
{
  auto begraceful =
      GENERATE(fetchpp::GracefulShutdown::Yes, fetchpp::GracefulShutdown::No);
  SECTION("test interrupt")
  {
    test::helpers::fake_server server(worker(1).ex);
    fetchpp::tcp_async_transport ts(worker(2).ex, 5s);

    auto url = tcp_endpoint_to_url(server.local_endpoint());
    url.set_pathname("/get");

    auto endpoint = fetchpp::detail::to_endpoint<false>(url);
    auto future = ts.async_connect(endpoint, boost::asio::use_future);
    auto fake_session = server.async_accept(fetchpp::net::use_future).get();
    REQUIRE_NOTHROW(future.get());

    auto const request = fetchpp::http::request(fetchpp::http::verb::get, url);
    fetchpp::http::response response;
    auto process_fut = fetchpp::async_process_one(
        ts, request, response, boost::asio::use_future);
    REQUIRE_NOTHROW(fake_session.async_receive(net::use_future).get());
    {
      CAPTURE(begraceful);
      REQUIRE_NOTHROW(ts.async_close(begraceful, net::use_future).get());
    }
    REQUIRE_THROWS(process_fut.get());
    REQUIRE_NOTHROW(ts.async_close(boost::asio::use_future).get());
  }
}

TEST_CASE_METHOD(ioc_fixture,
                 "transport one tcp over timeout",
                 "[transport][http][delay]")
{
  auto const url = URL("delay/5"_http);

  fetchpp::tcp_async_transport ts(ioc.get_executor(), 1s);
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

  fetchpp::ssl_async_transport ts(ioc.get_executor(), 1s, context);
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
  auto begraceful =
      GENERATE(fetchpp::GracefulShutdown::Yes, fetchpp::GracefulShutdown::No);
  SECTION("closing test")
  {
    auto const url = URL("get"_https);
    ssl::context context(ssl::context::tlsv12_client);

    fetchpp::ssl_async_transport ts(ioc.get_executor(), 5s, context);
    auto endpoint = fetchpp::detail::to_endpoint<true>(url);
    REQUIRE_NOTHROW(ts.async_connect(endpoint, boost::asio::use_future).get());
    auto const request = fetchpp::http::request(fetchpp::http::verb::get, url);
    fetchpp::http::response response;
    REQUIRE_NOTHROW(fetchpp::async_process_one(
                        ts, request, response, boost::asio::use_future)
                        .get());
    {
      CAPTURE(begraceful);
      REQUIRE_NOTHROW(ts.async_close(boost::asio::use_future).get());
    }
    REQUIRE_NOTHROW(ts.async_connect(endpoint, boost::asio::use_future).get());
  }
}
