#include <fetchpp/http/content_type.hpp>
#include <fetchpp/http/request.hpp>
#include <fetchpp/http/url.hpp>

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/vector_body.hpp>

#include <nlohmann/json.hpp>

#include <catch2/catch.hpp>

using namespace fetchpp::http::url_literals;
using fetchpp::string_view;

class ContentTypeMatcher : public Catch::MatcherBase<string_view>
{
  fetchpp::http::content_type ref_ct;

public:
  ContentTypeMatcher(string_view type,
                     string_view charset = "",
                     string_view boundary = "")
    : ref_ct(fetchpp::http::content_type(type, charset, boundary))
  {
  }

  [[nodiscard]] bool match(string_view const& i) const noexcept override
  try
  {
    return ref_ct == fetchpp::http::content_type::parse(i);
  }
  catch (std::domain_error const& ex)
  {
    return false;
  }

  std::string describe() const override
  {
    return std::string("equals to: ") + to_string(ref_ct);
  }
};

inline ContentTypeMatcher Equals(string_view type,
                                 string_view charset = "",
                                 string_view boundary = "")
{
  return ContentTypeMatcher(type, charset, boundary);
}

TEST_CASE("parsing content_type", "[content_type]")
{
  using fetchpp::http::content_type;
  using namespace std::literals::string_view_literals;
  CHECK_THAT("application/json; charset=UTF8",
             Equals("application/json", "UTF8"));
  CHECK_THAT("application/json; boundary=--paf--",
             Equals("application/json", "", "--paf--"));
  CHECK_THAT(("application/json; charset=ascii; boundary=--paf--"),
             Equals("application/json", "ascii", "--paf--"));
  REQUIRE_NOTHROW(
      content_type::parse("application/json; charset=ascii; boundary=--paf--"));
  /// spurious ; at the end!
  CHECK_THAT("application/json; charset=ascii; boundary=--paf--;",
             !(Equals("application/json", "ascii", "--paf--")));
  CHECK_THAT("application/json;          charset=ascii; boundary=--paf--",
             (Equals("application/json", "ascii", "--paf--")));
}

TEST_CASE("request set host and port", "[custom_port]")
{
  using namespace fetchpp;
  {
    auto const request =
        http::request(http::verb::get, "http://domain.com:2121/target"_url);
    REQUIRE(request[http::field::host] == "domain.com:2121");
  }
  {
    auto const request =
        http::request(http::verb::get, "http://domain.com:4242/target"_url);
    REQUIRE(request[http::field::host] == "domain.com:4242");
  }
}

TEST_CASE("request a complex url", "[request]")
{
  auto const req = fetchpp::http::request(
      fetchpp::http::verb::get,
      "http://toto.com/hello/world?language=en_us#GB"_url);
  REQUIRE(req.target() == "/hello/world?language=en_us#GB");
}

TEST_CASE("request a json body", "[request]")
{
  nlohmann::json v{{"key", "value"}};
  auto req =
      fetchpp::http::request(fetchpp::http::verb::get, "http://toto.tv"_url);
  req.content(v.dump());
  req.set(fetchpp::http::content_type("application/json", "utf8"));
}

TEST_CASE("request a buffer body", "[request]")
{
  std::vector vec = {1, 2, 3, 4, 5};
  auto req =
      fetchpp::http::request(fetchpp::http::verb::get, "http://toto.tv"_url);
  req.content(fetchpp::net::buffer(vec));
  req.set(fetchpp::http::content_type("application/json", "utf8"));
}
