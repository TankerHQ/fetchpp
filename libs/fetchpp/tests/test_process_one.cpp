#include <catch2/catch.hpp>

#include <fetchpp/core/process_one.hpp>
#include <fetchpp/version.hpp>

#include <fetchpp/http/field.hpp>
#include <fetchpp/http/request.hpp>
#include <fetchpp/http/response.hpp>

#include <boost/asio/use_future.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/span_body.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>

#include "helpers/fake_server.hpp"
#include "helpers/https_connect.hpp"
#include "helpers/server_certificate.hpp"
#include "helpers/test_domain.hpp"
#include "helpers/worker_fixture.hpp"

#include <nlohmann/json.hpp>

using namespace test::helpers;
using namespace test::helpers::http_literals;
using test::helpers::worker_fixture;
using URL = fetchpp::http::url;

namespace fetchpp
{
namespace ssl = net::ssl;
}
template <typename AsyncStream = bb::tcp_stream>
using ssl_stream = fetchpp::beast::ssl_stream<fetchpp::beast::tcp_stream>;

TEST_CASE_METHOD(worker_fixture,
                 "http GET with string_body",
                 "[http][string_body][process_one][async]")
{
  fetchpp::beast::tcp_stream stream(worker().ex);
  auto url = URL("get"_http);
  http_connect(stream, url);

  auto request = fetchpp::beast::http::request<fetchpp::http::string_body>(
      fetchpp::http::verb::get, "/get", 11);
  request.set(boost::beast::http::field::host, url.host());
  request.prepare_payload();

  fetchpp::beast::flat_buffer buffer;
  fetchpp::beast::http::response<fetchpp::http::string_body> response;
  REQUIRE_NOTHROW(fetchpp::async_process_one(
                      stream, buffer, request, response, net::use_future)
                      .get());
  REQUIRE(response.result_int() == 200);
  REQUIRE(response.at(fetchpp::http::field::content_type) ==
          "application/json");
}

TEST_CASE_METHOD(worker_fixture,
                 "https GET with string_body",
                 "[https][string_body][process_one][async]")
{
  fetchpp::ssl::context context(fetchpp::ssl::context::tlsv12_client);
  fetchpp::beast::ssl_stream<fetchpp::beast::tcp_stream> stream(worker().ex,
                                                                context);
  auto url = URL("/get"_https);
  https_connect(stream, url);

  auto request = fetchpp::beast::http::request<fetchpp::http::string_body>(
      fetchpp::http::verb::get, "/get", 11);
  request.set(boost::beast::http::field::host, url.host());
  request.prepare_payload();

  fetchpp::beast::flat_buffer buffer;
  fetchpp::beast::http::response<fetchpp::http::string_body> response;
  REQUIRE_NOTHROW(fetchpp::async_process_one(
                      stream, buffer, request, response, net::use_future)
                      .get());
  REQUIRE(response.result_int() == 200);
  REQUIRE(to_status_class(response.result()) ==
          boost::beast::http::status_class::successful);
  REQUIRE(response.at(fetchpp::http::field::content_type) ==
          "application/json");
}

TEST_CASE_METHOD(worker_fixture,
                 "http GET with poly body",
                 "[http][poly_body][process_one][async]")
{
  fetchpp::beast::tcp_stream stream(worker().ex);
  auto url = URL("get"_http);
  http_connect(stream, url);

  auto request = fetchpp::http::request(fetchpp::http::verb::get, url);
  fetchpp::http::response response;
  fetchpp::beast::flat_buffer buffer;
  REQUIRE_NOTHROW(fetchpp::async_process_one(
                      stream, buffer, request, response, net::use_future)
                      .get());
  REQUIRE(response.ok());
  REQUIRE(response.is_json());
  REQUIRE_NOTHROW(response.text());
  REQUIRE_NOTHROW(response.json());
}

TEST_CASE_METHOD(worker_fixture,
                 "https GET with poly body",
                 "[https][poly_body][process_one][async]")
{
  fetchpp::ssl::context context(fetchpp::ssl::context::tlsv12_client);
  fetchpp::beast::ssl_stream<fetchpp::beast::tcp_stream> stream(worker().ex,
                                                                context);
  auto const url = URL("get"_https);
  https_connect(stream, url);

  auto request = fetchpp::http::request(fetchpp::http::verb::get, url);
  request.accept("application/json");
  fetchpp::http::response response;
  fetchpp::beast::flat_buffer buffer;
  REQUIRE_NOTHROW(fetchpp::async_process_one(
                      stream, buffer, request, response, net::use_future)
                      .get());
  REQUIRE(response.ok());
  REQUIRE(response.is_json());
  REQUIRE_NOTHROW(response.text());
  REQUIRE_NOTHROW(response.json());
}

TEST_CASE_METHOD(worker_fixture,
                 "https connection management",
                 "[fake][https][poly_body][process_one][async]")
{
  net::ssl::context context(fetchpp::ssl::context::tlsv12);
  test::helpers::load_server_certificate(context);

  ssl_stream<> client(worker(0).ex, context);
  test::helpers::fake_server server(worker(1).ex);

  auto client_fut =
      async_ssl_connect(client, server.local_endpoint(), net::use_future);
  auto fake_session = server.async_accept(context, net::use_future).get();
  REQUIRE_NOTHROW(client_fut.get());

  auto url = URL("anything"_http);
  auto request = fetchpp::http::request(fetchpp::http::verb::get, url);
  request.accept(fetchpp::http::content_type("application/json"));
  fetchpp::http::response response;
  fetchpp::beast::flat_buffer buffer;
  auto fut = fetchpp::async_process_one(
      client, buffer, request, response, net::use_future);

  SECTION("server replies gracefully")
  {
    REQUIRE_NOTHROW(
        fake_session.async_reply_back(bb::http::status::ok, net::use_future)
            .get());

    REQUIRE_NOTHROW(fut.get());
    REQUIRE(response.ok());
    REQUIRE(response.is_json());
  }
  SECTION("server closes connection immediately")
  {
    fake_session.close();
    REQUIRE_THROWS(fut.get());
  }
  SECTION("server closes connection after request transmission")
  {
    REQUIRE_NOTHROW(fake_session.async_receive(net::use_future).get());
    fake_session.close();

    REQUIRE_THROWS(fut.get());
  }
  SECTION("client closes connection after request transmission")
  {
    REQUIRE_NOTHROW(fake_session.async_receive(net::use_future).get());
    bb::get_lowest_layer(client).close();
    REQUIRE_THROWS(fut.get());
  }
  SECTION("server closes connection when reading request")
  {
    fake_session.async_receive_some(40, net::use_future).get();
    fake_session.close();
    REQUIRE_THROWS(fut.get());
  }
  SECTION("server closes connection when sending response")
  {
    auto request = fake_session.async_receive(net::use_future).get();
    auto data = nlohmann::json({"data", request.body()}).dump();
    auto res =
        fake_session.response(bb::http::status::ok, "application/json", data);
    fake_session.async_send_some(std::move(res), 5, net::use_future);
    fake_session.close();
    REQUIRE_THROWS(fut.get());
  }
}

TEST_CASE_METHOD(worker_fixture,
                 "http connection management",
                 "[fake][http][poly_body][process_one][async]")
{
  fetchpp::beast::tcp_stream client(worker(0).ex);
  test::helpers::fake_server server(worker(1).ex);

  auto future = client.async_connect(server.local_endpoint(), net::use_future);
  auto fake_session =
      server.async_accept<bb::tcp_stream>(net::use_future).get();
  REQUIRE_NOTHROW(future.get());

  auto url = URL("anything"_http);
  auto request = fetchpp::http::request(fetchpp::http::verb::get, url);
  request.accept(fetchpp::http::content_type("application/json"));
  fetchpp::http::response response;
  fetchpp::beast::flat_buffer buffer;
  auto fut = fetchpp::async_process_one(
      client, buffer, request, response, net::use_future);

  SECTION("server replies gracefully")
  {
    REQUIRE_NOTHROW(
        fake_session.async_reply_back(bb::http::status::ok, net::use_future)
            .get());

    REQUIRE_NOTHROW(fut.get());
    REQUIRE(response.ok());
    REQUIRE(response.is_json());
  }
  SECTION("server closes connection immediately")
  {
    fake_session.close();
    REQUIRE_THROWS(fut.get());
  }
  SECTION("server closes connection after request transmission")
  {
    REQUIRE_NOTHROW(fake_session.async_receive(net::use_future).get());
    fake_session.close();

    REQUIRE_THROWS(fut.get());
  }
  SECTION("client closes connection after request transmission")
  {
    REQUIRE_NOTHROW(fake_session.async_receive(net::use_future).get());
    bb::get_lowest_layer(client).close();
    REQUIRE_THROWS(fut.get());
  }
  SECTION("server closes connection when reading request")
  {
    fake_session.async_receive_some(40, net::use_future).get();
    fake_session.close();
    REQUIRE_THROWS(fut.get());
  }
  SECTION("server closes connection when sending response")
  {
    auto request = fake_session.async_receive(net::use_future).get();
    auto data = nlohmann::json({"data", request.body()}).dump();
    auto res =
        fake_session.response(bb::http::status::ok, "application/json", data);
    fake_session.async_send_some(std::move(res), 5, net::use_future);
    fake_session.close();
    REQUIRE_THROWS(fut.get());
  }
}
