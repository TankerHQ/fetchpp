#include <fetchpp/http/content_type.hpp>
#include <fetchpp/http/request.hpp>
#include <fetchpp/http/url.hpp>

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/vector_body.hpp>

#include <nlohmann/json.hpp>

#include <sstream>

#include <catch2/catch.hpp>

using namespace fetchpp::http::url_literals;

class ContentTypeMatcher : public Catch::MatcherBase<std::string_view>
{
  fetchpp::http::content_type ref_ct;

public:
  template <typename... Args>
  ContentTypeMatcher(Args&&... args)
    : ref_ct(fetchpp::http::content_type(std::forward<Args>(args)...))
  {
  }

  [[nodiscard]] bool match(std::string_view const& i) const noexcept override
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
    std::ostringstream ss;
    ss << "equals to: " << ref_ct;
    return ss.str();
  }
};

template <typename... Args>
inline ContentTypeMatcher Equals(Args&&... args)
{
  return ContentTypeMatcher(std::forward<Args>(args)...);
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
  REQUIRE_NOTHROW(content_type::parse(
      "application/json; charset=ascii; boundary=--paf--"sv));
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
  req.content_type(fetchpp::http::content_type("application/json", "utf8"));
}

TEST_CASE("request a buffer body", "[request]")
{
  std::vector vec = {1, 2, 3, 4, 5};
  auto req =
      fetchpp::http::request(fetchpp::http::verb::get, "http://toto.tv"_url);
  req.content(fetchpp::net::buffer(vec));
  req.content_type(fetchpp::http::content_type("application/json", "utf8"));
}
