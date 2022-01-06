#include <fetchpp/core/session.hpp>

#include <fetchpp/core/ssl_transport.hpp>
#include <fetchpp/http/request.hpp>
#include <fetchpp/http/response.hpp>

#include <fetchpp/core/detail/endpoint.hpp>

#include "helpers/fake_server.hpp"
#include "helpers/format.hpp"
#include "helpers/ioc_fixture.hpp"
#include "helpers/match_exception.hpp"
#include "helpers/test_domain.hpp"
#include "helpers/worker_fixture.hpp"

#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/use_future.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>

#include <fetchpp/alias/net.hpp>

#include <array>
#include <deque>
#include <string>

#include <catch2/catch.hpp>
#include <fmt/format.h>

using namespace test::helpers::http_literals;
using namespace std::chrono_literals;

using test::helpers::HasErrorCode;
using test::helpers::ioc_fixture;
using test::helpers::worker_fixture;

namespace ssl = fetchpp::net::ssl;
namespace net = boost::asio;
namespace bb = boost::beast;

using AsyncStream = fetchpp::beast::ssl_stream<fetchpp::beast::tcp_stream>;
using boost::asio::use_future;
using URL = fetchpp::http::url;
using string_view = fetchpp::string_view;

namespace
{
auto tcp_endpoint_to_url(net::ip::tcp::endpoint const& endpoint,
                         string_view path = "",
                         string_view protocol = "http")
{
  return fmt::format("{}://{}:{}{}",
                     protocol,
                     endpoint.address().to_string(),
                     endpoint.port(),
                     path);
}
}

CATCH_REGISTER_ENUM(fetchpp::GracefulShutdown,
                    fetchpp::GracefulShutdown::Yes,
                    fetchpp::GracefulShutdown::No)

namespace fetchpp
{
static std::ostream& operator<<(std::ostream& os,
                                fetchpp::GracefulShutdown const& g)
{
  os << ((g == fetchpp::GracefulShutdown::Yes) ? "Yes" : "No");
  return os;
}
}

template <>
struct Catch::StringMaker<fetchpp::beast::error_code>
{
  static std::string convert(fetchpp::beast::error_code const& ec)
  {
    return fmt::format("error code: {2}:{0}, \"{1}\"",
                       ec.value(),
                       ec.message(),
                       ec.category().name());
  }
};

TEST_CASE_METHOD(worker_fixture,
                 "session executes multiples request",
                 "[session][push][fake]")
{
  test::helpers::fake_server server(worker(1).ex);
  auto dest = tcp_endpoint_to_url(server.local_endpoint(), "/get", "http");
  auto session = fetchpp::session(
      fetchpp::detail::to_endpoint<false>(URL(dest)), worker().ex, 30s);

  auto request = fetchpp::http::request(fetchpp::http::verb::get,
                                        fetchpp::http::url("get"_http));
  std::vector<std::tuple<std::future<void>, fetchpp::http::response>> results{
      10};
  for (auto& [fut, response] : results)
    fut = session.push_request(request, response, boost::asio::use_future);
  for (auto& [fut, response] : results)
  {
    auto fake_session = server.async_accept(fetchpp::net::use_future).get();
    REQUIRE_NOTHROW(
        fake_session.async_reply_back(bb::http::status::ok, net::use_future)
            .get());
    REQUIRE_NOTHROW(fut.get());
    REQUIRE(response.result_int() == 200);
  }
}

TEST_CASE_METHOD(worker_fixture,
                 "completion is executed on the correct executor",
                 "[session][push][fake]")
{
  test::helpers::fake_server server(worker(1).ex);
  auto dest = tcp_endpoint_to_url(server.local_endpoint(), "/get", "http");
  auto session = fetchpp::session(
      fetchpp::detail::to_endpoint<false>(URL(dest)), worker(2).ex, 30s);
  auto request = fetchpp::http::request(fetchpp::http::verb::get,
                                        fetchpp::http::url("get"_http));
  {
    auto resp = fetchpp::http::response{};
    std::promise<std::thread::id> isdone;
    auto waitforit = isdone.get_future();
    session.push_request(request, resp, [&](bb::error_code ec) {
      if (ec)
        return isdone.set_exception(
            std::make_exception_ptr(boost::system::system_error(ec)));
      isdone.set_value(std::this_thread::get_id());
    });
    auto fake_session = server.async_accept(net::use_future).get();
    REQUIRE_NOTHROW(
        fake_session.async_reply_back(bb::http::status::ok, net::use_future)
            .get());
    REQUIRE(waitforit.get() == worker(2).worker.get_id());
  }
  {
    auto resp = fetchpp::http::response{};
    std::promise<std::thread::id> isdone;
    auto waitforit = isdone.get_future();
    session.push_request(
        request, resp, net::bind_executor(worker(0).ex, [&](bb::error_code ec) {
          if (ec)
            return isdone.set_exception(
                std::make_exception_ptr(boost::system::system_error(ec)));
          isdone.set_value(std::this_thread::get_id());
        }));
    auto fake_session = server.async_accept(net::use_future).get();
    REQUIRE_NOTHROW(
        fake_session.async_reply_back(bb::http::status::ok, net::use_future)
            .get());
    REQUIRE(waitforit.get() == worker(0).worker.get_id());
  }
  REQUIRE_NOTHROW(session.async_stop(net::use_future).get());
}

TEST_CASE_METHOD(worker_fixture,
                 "session is running while pushing multiple requests",
                 "[session][push][fake]")
{
  test::helpers::fake_server server(worker(1).ex);
  auto dest = tcp_endpoint_to_url(server.local_endpoint(), "/get", "http");
  auto session = fetchpp::session(
      fetchpp::detail::to_endpoint<false>(URL(dest)), worker(2).ex, 30s);
  auto fut = session.async_start(net::use_future);
  auto fake_session = server.async_accept(net::use_future).get();
  REQUIRE_NOTHROW(fut.get());
  auto request = fetchpp::http::request(fetchpp::http::verb::get,
                                        fetchpp::http::url("get"_http));

  std::deque<std::tuple<std::future<void>, fetchpp::http::response>> results{6};
  for (auto& [fut, response] : results)
    fut = session.push_request(request, response, boost::asio::use_future);

  for (auto& [fut, response] : results)
  {
    REQUIRE_NOTHROW(
        fake_session.async_reply_back(bb::http::status::ok, net::use_future)
            .get());
    REQUIRE_NOTHROW(fut.get());
  }
  REQUIRE_NOTHROW(session.async_stop(net::use_future).get());
}

TEST_CASE_METHOD(worker_fixture,
                 "session is interrupted once while pushing multiple requests",
                 "[session][interrupt][fake]")
{
  test::helpers::fake_server server(worker(1).ex);
  auto dest = tcp_endpoint_to_url(server.local_endpoint(), "/get", "http");
  auto session = fetchpp::session(
      fetchpp::detail::to_endpoint<false>(URL(dest)), worker(0).ex, 30s);

  auto request = fetchpp::http::request(fetchpp::http::verb::get,
                                        fetchpp::http::url("get"_http));
  std::deque<std::tuple<std::future<void>, fetchpp::http::response>> results{6};
  for (auto& [fut, response] : results)
    fut = session.push_request(request, response, net::use_future);
  // receive the first one
  auto fake_session = server.async_accept(fetchpp::net::use_future).get();
  {
    auto& [fut, response] = results.front();
    REQUIRE_NOTHROW(
        fake_session.async_reply_back(bb::http::status::ok, net::use_future)
            .get());
    REQUIRE_NOTHROW(fut.get());
    REQUIRE(response.result_int() == 200);
    results.pop_front();
  }
  {
    auto& [fut, response] = results.front();
    REQUIRE_NOTHROW(fake_session.async_receive_some(10, net::use_future).get());
    REQUIRE_NOTHROW(fake_session.close());
    fake_session = server.async_accept(fetchpp::net::use_future).get();
    REQUIRE_NOTHROW(
        fake_session.async_reply_back(bb::http::status::ok, net::use_future)
            .get());
    REQUIRE_NOTHROW(fut.get());
    REQUIRE(response.result_int() == 200);
    results.pop_front();
  }
  for (auto i = 0u; i < results.size(); ++i)
  {
    INFO(fmt::format("processing results {}/{}", i + 1, results.size()));
    auto& [fut, response] = results[i];
    REQUIRE_NOTHROW(
        fake_session.async_reply_back(bb::http::status::ok, net::use_future)
            .get());
    REQUIRE_NOTHROW(fut.get());
    REQUIRE(response.result_int() == 200);
  }
  REQUIRE_NOTHROW(session.async_stop(net::use_future).get());
}

TEST_CASE_METHOD(worker_fixture,
                 "session is interrupted twice on the same request while "
                 "pushing multiple requests",
                 "[session][interrupt][fake]")
{
  test::helpers::fake_server server(worker(1).ex);
  auto dest = tcp_endpoint_to_url(server.local_endpoint(), "/get", "http");
  auto session = fetchpp::session(
      fetchpp::detail::to_endpoint<false>(URL(dest)), worker().ex, 30s);

  auto request = fetchpp::http::request(fetchpp::http::verb::get,
                                        fetchpp::http::url("get"_http));
  std::deque<std::tuple<std::future<void>, fetchpp::http::response>> results{5};
  for (auto& [fut, response] : results)
    fut = session.push_request(request, response, net::use_future);
  auto fake_session = server.async_accept(fetchpp::net::use_future).get();
  {
    auto& [fut, response] = results.front();
    REQUIRE_NOTHROW(
        fake_session.async_reply_back(bb::http::status::ok, net::use_future)
            .get());
    REQUIRE_NOTHROW(fut.get());
    INFO("the first request goes smoothly");
    REQUIRE(response.result_int() == 200);
    results.pop_front();
  }
  {
    auto& [fut, response] = results.front();
    REQUIRE_NOTHROW(fake_session.async_receive_some(10, net::use_future).get());
    UNSCOPED_INFO("interrupting the first time");
    REQUIRE_NOTHROW(fake_session.close());
    fake_session = server.async_accept(fetchpp::net::use_future).get();
    REQUIRE_NOTHROW(fake_session.async_receive_some(10, net::use_future).get());
    UNSCOPED_INFO("interrupting the second time");
    REQUIRE_NOTHROW(fake_session.close());
    REQUIRE_THROWS_MATCHES(fut.get(),
                           boost::system::system_error,
                           HasErrorCode(boost::asio::error::connection_reset));
    results.pop_front();
  }
  INFO("the remaining request are aborted");
  for (auto& [fut, response] : results)
    REQUIRE_THROWS_MATCHES(fut.get(),
                           boost::system::system_error,
                           HasErrorCode(boost::asio::error::operation_aborted));
}

TEST_CASE_METHOD(worker_fixture,
                 "session is interrupted twice on a different request while "
                 "pushing multiple requests",
                 "[session][interrupt][fake]")
{
  test::helpers::fake_server server(worker(1).ex);
  auto dest = tcp_endpoint_to_url(server.local_endpoint(), "/get", "http");
  auto session = fetchpp::session(
      fetchpp::detail::to_endpoint<false>(URL(dest)), worker().ex, 30s);

  auto request = fetchpp::http::request(fetchpp::http::verb::get,
                                        fetchpp::http::url("get"_http));
  std::deque<std::tuple<std::future<void>, fetchpp::http::response>> results{5};
  for (auto& [fut, response] : results)
    fut = session.push_request(request, response, net::use_future);
  auto fake_session = server.async_accept(fetchpp::net::use_future).get();
  {
    auto& [fut, response] = results.front();
    REQUIRE_NOTHROW(
        fake_session.async_reply_back(bb::http::status::ok, net::use_future)
            .get());
    REQUIRE_NOTHROW(fut.get());
    INFO("the first request goes smoothly");
    REQUIRE(response.result_int() == 200);
    results.pop_front();
  }
  {
    auto& [fut, response] = results.front();
    REQUIRE_NOTHROW(fake_session.async_receive_some(10, net::use_future).get());
    UNSCOPED_INFO("interrupting the first time");
    REQUIRE_NOTHROW(fake_session.close());
    fake_session = server.async_accept(fetchpp::net::use_future).get();

    REQUIRE_NOTHROW(
        fake_session.async_reply_back(bb::http::status::ok, net::use_future)
            .get());
    REQUIRE_NOTHROW(fut.get());
    INFO("the second request goes smoothly");
    REQUIRE(response.result_int() == 200);
    results.pop_front();
  }
  {
    auto& [fut, response] = results.front();
    REQUIRE_NOTHROW(fake_session.async_receive_some(10, net::use_future).get());
    UNSCOPED_INFO("interrupting the second time");
    REQUIRE_NOTHROW(fake_session.close());
    fake_session = server.async_accept(fetchpp::net::use_future).get();

    REQUIRE_NOTHROW(
        fake_session.async_reply_back(bb::http::status::ok, net::use_future)
            .get());
    REQUIRE_NOTHROW(fut.get());
    INFO("the third request goes smoothly");
    REQUIRE(response.result_int() == 200);
    results.pop_front();
  }
  INFO("the remaining request proceed normally");
  for (auto& [fut, response] : results)
  {
    REQUIRE_NOTHROW(
        fake_session.async_reply_back(bb::http::status::ok, net::use_future)
            .get());
    REQUIRE_NOTHROW(fut.get());
    REQUIRE(response.result_int() == 200);
  }
}

TEST_CASE_METHOD(worker_fixture,
                 "session is stopped while running multiple requests",
                 "[session][stop][fake]")
{
  test::helpers::fake_server server(worker(1).ex);
  auto dest = tcp_endpoint_to_url(server.local_endpoint(), "/get", "http");
  auto session = fetchpp::session(
      fetchpp::detail::to_endpoint<false>(URL(dest)), worker(2).ex, 30s);
  auto fut = session.async_start(net::use_future);
  auto fake_session = server.async_accept(net::use_future).get();
  REQUIRE_NOTHROW(fut.get());
  auto request = fetchpp::http::request(fetchpp::http::verb::get,
                                        fetchpp::http::url("get"_http));

  std::deque<std::tuple<std::future<void>, fetchpp::http::response>> results{6};
  for (auto& [fut, response] : results)
    fut = session.push_request(request, response, boost::asio::use_future);

  for (auto i = 0u; i < 3; ++i)
  {
    auto& [fut, response] = results.front();
    REQUIRE_NOTHROW(
        fake_session.async_reply_back(bb::http::status::ok, net::use_future)
            .get());
    REQUIRE_NOTHROW(fut.get());
    results.pop_front();
  }
  REQUIRE_NOTHROW(session.async_stop(net::use_future).get());

  for (auto& [fut, response] : results)
    REQUIRE_THROWS(fut.get());
  REQUIRE_NOTHROW(session.async_stop(net::use_future).get());
}

// ======================= fake

TEST_CASE_METHOD(ioc_fixture,
                 "session fails to auto connect on bad domain",
                 "[session][connect]")
{
  ssl::context context(ssl::context::tlsv12_client);
  auto url = fetchpp::http::url("https://127.0.0.2:444");
  auto session = fetchpp::session(
      fetchpp::secure_endpoint("127.0.0.2", 444), ex, 3s, context);
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
      fetchpp::detail::to_endpoint<true>(url), ex, 30s, context);

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
                 "sessions abort when connection times out",
                 "[session][delay]")
{
  ssl::context sslc(ssl::context::tlsv12_client);
  auto const url = fetchpp::http::url("delay/3"_http);
  auto request = fetchpp::http::request(fetchpp::http::verb::get, url);
  auto session = fetchpp::session(fetchpp::secure_endpoint("10.255.255.1", 444),
                                  ioc.get_executor(),
                                  1s,
                                  sslc);
  fetchpp::http::response response;
  REQUIRE_THROWS_MATCHES(
      session.push_request(request, response, fetchpp::net::use_future).get(),
      boost::system::system_error,
      HasErrorCode(boost::beast::error::timeout));
}

TEST_CASE_METHOD(ioc_fixture,
                 "session aborts a request that takes too much time",
                 "[session][delay]")
{
  auto const url = fetchpp::http::url("delay/4"_http);
  auto session =
      fetchpp::session(fetchpp::detail::to_endpoint<false>(url), ex, 1s);
  auto request = fetchpp::http::request(fetchpp::http::verb::get, url);
  fetchpp::http::response response;
  REQUIRE_THROWS_MATCHES(
      session.push_request(request, response, fetchpp::net::use_future).get(),
      boost::system::system_error,
      HasErrorCode(boost::beast::error::timeout));
}

TEST_CASE_METHOD(ioc_fixture,
                 "session executes two requests pushed",
                 "[session][push]")
{
  ssl::context context(ssl::context::tlsv12_client);
  auto const url = fetchpp::http::url("get"_https);
  auto session =
      fetchpp::session(fetchpp::detail::to_endpoint(url), ex, 30s, context);
  auto request = fetchpp::http::request(fetchpp::http::verb::get,
                                        fetchpp::http::url("get"_https));
  {
    fetchpp::http::response response;
    session.push_request(request, response, boost::asio::use_future).get();
    REQUIRE(response.result_int() == 200);
  }
  {
    fetchpp::http::response response;
    session.push_request(request, response, boost::asio::use_future).get();
    REQUIRE(response.result_int() == 200);
  }
  auto fut2 = session.async_stop(boost::asio::use_future);
  REQUIRE_NOTHROW(fut2.get());
}

TEST_CASE_METHOD(ioc_fixture,
                 "session executes multiple requests pushed with some delay",
                 "[session][http][push][delay]")
{
  auto const urlget = fetchpp::http::url("get"_http);
  auto const urldelay = fetchpp::http::url("delay/3"_http);
  auto session =
      fetchpp::session(fetchpp::detail::to_endpoint<false>(urlget), ex, 2s);

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
  REQUIRE(!session.has_tasks());
}

TEST_CASE_METHOD(ioc_fixture,
                 "session closes while connecting",
                 "[session][https][close][delay]")
{
  auto context = ssl::context(ssl::context::tlsv12_client);
  auto const urldelay = fetchpp::http::url("delay/1"_https);
  auto session = fetchpp::session(
      fetchpp::detail::to_endpoint<true>(urldelay), ex, 5s, context);

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
    REQUIRE(!session.has_tasks());
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
  auto session = fetchpp::session(tunnel, ex, 30s, ctx);

  auto request = fetchpp::http::request(fetchpp::http::verb::get,
                                        fetchpp::http::url("get"_https));
  {
    session.async_start(use_future).get();
    fetchpp::http::response response;
    session.push_request(request, response, use_future).get();
    REQUIRE(response.result_int() == 200);
  }
}

TEST_CASE_METHOD(
    worker_fixture,
    "session closes ungracefully while multiple request are executed",
    "[session][https][close][delay]")
{
  auto begraceful =
      GENERATE(fetchpp::GracefulShutdown::Yes, fetchpp::GracefulShutdown::No);
  DYNAMIC_SECTION("closing test gracefully: " << begraceful)
  {
    auto context = ssl::context(ssl::context::tlsv12_client);
    auto const urldelay = fetchpp::http::url("delay/3"_https);
    auto session =
        fetchpp::session(fetchpp::detail::to_endpoint<true>(urldelay),
                         worker(1).ex,
                         5s,
                         context);

    // wait for the connecton to settle
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
      std::deque<std::tuple<std::future<void>, fetchpp::http::response>>
          results(10);
      for (auto& [fut, res] : results)
        fut = session.push_request(delay, res, boost::asio::use_future);

      {
        REQUIRE_NOTHROW(
            session.async_stop(begraceful, fetchpp::net::use_future).get());
      }
      for (auto& [fut, res] : results)
        fut.wait();
      REQUIRE(!session.has_tasks());
    }
  }
}
