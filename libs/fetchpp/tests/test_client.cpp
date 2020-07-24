#include <fetchpp/client.hpp>

#include <fetchpp/http/json_body.hpp>

#include <boost/asio/use_future.hpp>

#include "helpers/format.hpp"
#include "helpers/ioc_fixture.hpp"
#include "helpers/match_exception.hpp"
#include "helpers/test_domain.hpp"

#include <catch2/catch.hpp>

#include <chrono>
#include <fmt/ostream.h>
#include <thread>

using namespace std::chrono_literals;

using namespace test::helpers;
using namespace test::helpers::http_literals;
using test::helpers::HasErrorCode;

TEST_CASE_METHOD(ioc_fixture, "client push one request", "[client][http]")
{
  fetchpp::client cl{ioc};
  auto const url = fetchpp::http::url("get"_https);
  auto request = fetchpp::http::make_request<
      fetchpp::http::request<fetchpp::http::json_body>>(
      fetchpp::http::verb::get, url);

  auto res = cl.async_fetch(std::move(request), boost::asio::use_future).get();
  REQUIRE(res.result_int() == 200);
  REQUIRE(cl.session_count() == 1);
  static_assert(fetchpp::net::is_executor<
                typename fetchpp::client::executor_type>::value);
}

TEST_CASE_METHOD(ioc_fixture, "client push two requests", "[client][http]")
{
  fetchpp::client cl{ioc};
  auto const url = fetchpp::http::url("get"_http);
  auto const surl = fetchpp::http::url("get"_https);
  auto request = fetchpp::http::make_request(fetchpp::http::verb::get, url);
  auto srequest = fetchpp::http::make_request(fetchpp::http::verb::get, surl);

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
  auto getrequest = fetchpp::http::make_request(fetchpp::http::verb::get, get);
  auto delayrequest =
      fetchpp::http::make_request(fetchpp::http::verb::get, delay);

  REQUIRE_THROWS_MATCHES(
      cl.async_fetch(delayrequest, boost::asio::use_future).get(),
      boost::system::system_error,
      HasErrorCode(boost::beast::error::timeout));
  auto res2 = cl.async_fetch(getrequest, boost::asio::use_future).get();
  REQUIRE(res2.result_int() == 200);
  REQUIRE(cl.session_count() == 2);
  static_assert(fetchpp::net::is_executor<
                typename fetchpp::client::executor_type>::value);
}

TEST_CASE_METHOD(ioc_fixture,
                 "https client stops while requesting",
                 "[client][https][delay]")
{
  using fetchpp::http::url;

  fetchpp::client cl{ioc, 5s};
  auto getrequest =
      fetchpp::http::make_request(fetchpp::http::verb::get, url("get"_https));
  auto delayrequest = fetchpp::http::make_request(fetchpp::http::verb::get,
                                                  url("delay/1"_https));
  // wait for the connection to established
  std::vector<std::future<fetchpp::http::response>> futures;
  for (int i = 0; i < 10; ++i)
    futures.push_back(cl.async_fetch(getrequest, boost::asio::use_future));

  for (auto& future : futures)
    REQUIRE_NOTHROW(future.get());

  futures.clear();
  for (int i = 0; i < 10; ++i)
    futures.push_back(cl.async_fetch(delayrequest, boost::asio::use_future));

  REQUIRE_NOTHROW(cl.async_stop(boost::asio::use_future));

  for (auto& future : futures)
    future.wait();
  // we keep that to not destroy client before connections die
  std::this_thread::sleep_for(500ms);
}

TEST_CASE_METHOD(ioc_fixture,
                 "http client stops while requesting",
                 "[client][http][delay]")
{
  using fetchpp::http::url;

  fetchpp::client cl{ioc, 5s};
  auto getrequest =
      fetchpp::http::make_request(fetchpp::http::verb::get, url("get"_http));
  auto delayrequest = fetchpp::http::make_request(fetchpp::http::verb::get,
                                                  url("delay/1"_http));
  // wait for the connection to established
  std::vector<std::future<fetchpp::http::response>> futures;
  for (int i = 0; i < 10; ++i)
    futures.push_back(cl.async_fetch(getrequest, boost::asio::use_future));

  for (auto& future : futures)
    REQUIRE_NOTHROW(future.get());

  futures.clear();
  for (int i = 0; i < 10; ++i)
    futures.push_back(cl.async_fetch(delayrequest, boost::asio::use_future));

  REQUIRE_NOTHROW(cl.async_stop(boost::asio::use_future));

  for (auto& future : futures)
    future.wait();
}

TEST_CASE_METHOD(ioc_fixture,
                 "https client stops while connecting",
                 "[client][https][delay]")
{
  fetchpp::client cl{ioc, 2s};
  auto const delay = fetchpp::http::url("delay/1"_https);
  auto delayrequest =
      fetchpp::http::make_request(fetchpp::http::verb::get, delay);

  std::vector<std::future<fetchpp::http::response>> futures;
  for (int i = 1; i < 5; ++i)
    futures.push_back(cl.async_fetch(delayrequest, boost::asio::use_future));

  REQUIRE_NOTHROW(cl.async_stop(boost::asio::use_future).get());

  for (auto& future : futures)
    future.wait();
}

TEST_CASE_METHOD(ioc_fixture,
                 "http client stops while connecting",
                 "[client][http][delay]")
{
  fetchpp::client cl{ioc, 2s};
  auto const delay = fetchpp::http::url("delay/1"_http);
  auto delayrequest =
      fetchpp::http::make_request(fetchpp::http::verb::get, delay);
  std::vector<std::future<fetchpp::http::response>> futures;
  for (int i = 1; i < 5; ++i)
    futures.push_back(cl.async_fetch(delayrequest, boost::asio::use_future));

  REQUIRE_NOTHROW(cl.async_stop(boost::asio::use_future));
  for (auto& future : futures)
    future.wait();
}