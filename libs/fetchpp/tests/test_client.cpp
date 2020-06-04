#include <fetchpp/client.hpp>

#include <boost/asio/use_future.hpp>

#include "helpers/format.hpp"
#include "helpers/ioc_fixture.hpp"
#include "helpers/test_domain.hpp"

#include <catch2/catch.hpp>

using namespace test::helpers;
using namespace test::helpers::http_literals;

TEST_CASE_METHOD(ioc_fixture, "client one", "[client][http]")
{
  fetchpp::client cl{ioc};
  auto const url = fetchpp::http::url::parse("get"_http);
  auto const surl = fetchpp::http::url::parse("get"_https);
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
