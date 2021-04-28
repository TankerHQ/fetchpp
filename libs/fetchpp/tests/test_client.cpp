#include <fetchpp/client.hpp>

#include <fetchpp/http/request.hpp>

#include <boost/asio/use_future.hpp>

#include "helpers/format.hpp"
#include "helpers/ioc_fixture.hpp"
#include "helpers/match_exception.hpp"
#include "helpers/test_domain.hpp"

#include <catch2/catch.hpp>

#include <chrono>

using namespace std::chrono_literals;

using namespace test::helpers;
using namespace test::helpers::http_literals;
using namespace std::string_view_literals;
using namespace fetchpp::http::url_literals;
using test::helpers::HasErrorCode;

namespace ssl = fetchpp::net::ssl;
using URL = fetchpp::http::url;

namespace fetchpp
{
static std::ostream& operator<<(std::ostream& os, GracefulShutdown const& g)
{
  os << ((g == fetchpp::GracefulShutdown::Yes) ? "Yes" : "No");
  return os;
}
}
static_assert(
    fetchpp::net::is_executor<fetchpp::client::default_executor_type>::value);
static_assert(
    fetchpp::net::is_executor<fetchpp::client::internal_executor_type>::value);

TEST_CASE_METHOD(ioc_fixture, "client push one request", "[client][http]")
{
  fetchpp::client cl{ioc};
  auto const url = fetchpp::http::url("get"_https);
  auto request = fetchpp::http::request(fetchpp::http::verb::get, url);

  auto res = cl.async_fetch(std::move(request), boost::asio::use_future).get();
  REQUIRE(res.result_int() == 200);
  REQUIRE(cl.session_count() == 1);
}

TEST_CASE_METHOD(ioc_fixture, "client push two requests", "[client][http]")
{
  fetchpp::client cl{ioc};
  auto const url = fetchpp::http::url("get"_http);
  auto const surl = fetchpp::http::url("get"_https);
  auto request = fetchpp::http::request(fetchpp::http::verb::get, url);
  auto srequest = fetchpp::http::request(fetchpp::http::verb::get, surl);

  auto res = cl.async_fetch(request, boost::asio::use_future).get();
  REQUIRE(res.result_int() == 200);
  auto res2 = cl.async_fetch(srequest, boost::asio::use_future).get();
  REQUIRE(res2.result_int() == 200);
  REQUIRE(cl.session_count() == 2);
}

TEST_CASE_METHOD(ioc_fixture, "client with delay", "[client][http][delay]")
{
  fetchpp::client cl{ioc, 2s};
  auto const get = fetchpp::http::url("get"_http);
  auto const delay = fetchpp::http::url("delay/3"_https);
  auto getrequest = fetchpp::http::request(fetchpp::http::verb::get, get);
  auto delayrequest = fetchpp::http::request(fetchpp::http::verb::get, delay);

  REQUIRE_THROWS_MATCHES(
      cl.async_fetch(delayrequest, boost::asio::use_future).get(),
      boost::system::system_error,
      HasErrorCode(boost::beast::error::timeout));
  auto res2 = cl.async_fetch(getrequest, boost::asio::use_future).get();
  REQUIRE(res2.result_int() == 200);
  REQUIRE(cl.session_count() == 2);
}

TEST_CASE_METHOD(ioc_fixture,
                 "client stops while requesting",
                 "[client][https][delay][stop]")
{
  auto const dstUrl =
      GENERATE(std::array{URL("get"_http), URL("delay/1"_http)},
               std::array{URL("get"_https), URL("delay/1"_https)});
  DYNAMIC_SECTION("requesting through " << std::get<0>(dstUrl).scheme())
  {
    auto const [url, delayUrl] = dstUrl;
    fetchpp::client cl{ioc, 5s};
    auto getrequest = fetchpp::http::request(fetchpp::http::verb::get, url);
    auto delayrequest =
        fetchpp::http::request(fetchpp::http::verb::get, delayUrl);
    // wait for the connection to be established
    std::deque<std::future<fetchpp::http::response>> futures{10};
    for (auto& fut : futures)
      fut = cl.async_fetch(getrequest, boost::asio::use_future);

    for (auto& future : futures)
      REQUIRE_NOTHROW(future.get());

    futures.clear();
    for (auto& fut : futures)
      fut = cl.async_fetch(delayrequest, boost::asio::use_future);

    std::future<void> closeFut;
    auto begraceful =
        GENERATE(fetchpp::GracefulShutdown::Yes, fetchpp::GracefulShutdown::No);
    DYNAMIC_SECTION("stopping gracefully: " << begraceful)
    {
      REQUIRE_NOTHROW(closeFut =
                          cl.async_stop(begraceful, boost::asio::use_future));
    }
    for (auto& future : futures)
      future.wait();
    REQUIRE_NOTHROW(closeFut.get());
  }
}

TEST_CASE_METHOD(ioc_fixture,
                 "client stops while requesting with multiple sessions",
                 "[client][https][delay][stop]")
{
  using fetchpp::http::url;
  auto const dstUrl = GENERATE(
      std::array{
          URL("get"_http), URL("delay/2"_https), URL("https://google.com")},
      std::array{
          URL("delay/2"_http), URL("get"_https), URL("https://google.com")});
  DYNAMIC_SECTION("requesting through " << std::get<0>(dstUrl).scheme())
  {
    fetchpp::client cl{ioc, 5s};

    std::deque<fetchpp::http::request> requests;
    for (auto const& u : dstUrl)
      requests.emplace_back(fetchpp::http::verb::get, u);
    // wait for the connection to be established
    std::deque<std::future<fetchpp::http::response>> futures;
    for (auto const& r : requests)
      futures.push_back(cl.async_fetch(r, net::use_future));

    for (auto& future : futures)
      REQUIRE_NOTHROW(future.get());

    futures.clear();
    for (int i = 0; i < 5; ++i)
      for (auto const& r : requests)
        futures.push_back(cl.async_fetch(r, net::use_future));

    std::future<void> closeFut;
    auto begraceful =
        GENERATE(fetchpp::GracefulShutdown::Yes, fetchpp::GracefulShutdown::No);
    DYNAMIC_SECTION("stopping gracefully: " << begraceful)
    {
      REQUIRE_NOTHROW(closeFut =
                          cl.async_stop(begraceful, boost::asio::use_future));
      for (auto& future : futures)
        future.wait();
      REQUIRE_NOTHROW(closeFut.get());
    }
  }
}

TEST_CASE_METHOD(ioc_fixture,
                 "client stops while connecting",
                 "[client][https][delay][stop]")
{
  auto const dst = URL(GENERATE("delay/1"_http, "delay/1"_https));
  DYNAMIC_SECTION("requesting through " << dst.scheme())
  {
    fetchpp::client cl{ioc, 2s};
    auto const delay = fetchpp::http::url("delay/1"_https);
    auto delayrequest = fetchpp::http::request(fetchpp::http::verb::get, delay);

    std::deque<std::future<fetchpp::http::response>> futures;
    for (int i = 1; i < 5; ++i)
      futures.push_back(cl.async_fetch(delayrequest, boost::asio::use_future));

    auto begraceful =
        GENERATE(fetchpp::GracefulShutdown::Yes, fetchpp::GracefulShutdown::No);
    DYNAMIC_SECTION("stopping gracefully: " << begraceful)
    {
      REQUIRE_NOTHROW(cl.async_stop(boost::asio::use_future).get());
    }

    for (auto& future : futures)
      future.wait();
  }
}

TEST_CASE_METHOD(ioc_fixture,
                 "http client uses proxy",
                 "[client][https][proxy]")
{
  fetchpp::client cl{ioc, 2s};
  auto const delay = fetchpp::http::url("get"_https);
  auto request = fetchpp::http::request(fetchpp::http::verb::get, delay);
  cl.add_proxy(fetchpp::http::proxy_scheme::https,
               fetchpp::http::proxy(get_test_proxy()));
  ssl::context ctx(ssl::context::tlsv12_client);

  auto res = cl.async_fetch(std::move(request), boost::asio::use_future).get();
  REQUIRE(res.result_int() == 200);
  auto begraceful =
      GENERATE(fetchpp::GracefulShutdown::Yes, fetchpp::GracefulShutdown::No);
  DYNAMIC_SECTION("stopping gracefully: " << begraceful)
  {
    REQUIRE_NOTHROW(cl.async_stop(boost::asio::use_future).get());
  }
}
