#include <fetchpp/client.hpp>

#include <boost/asio/use_future.hpp>

#include "helpers/format.hpp"
#include "helpers/ioc_fixture.hpp"
#include "helpers/match_exception.hpp"
#include "helpers/test_domain.hpp"

#include <catch2/catch.hpp>

#include <chrono>
#include <fmt/ostream.h>

using namespace std::chrono_literals;

using namespace test::helpers;
using namespace test::helpers::http_literals;
using test::helpers::HasErrorCode;

TEST_CASE_METHOD(ioc_fixture, "client one", "[client][http]")
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
  static_assert(fetchpp::net::is_executor<
                typename fetchpp::client::executor_type>::value);
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