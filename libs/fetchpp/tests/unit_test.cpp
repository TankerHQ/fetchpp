#include <fetchpp/content_type.hpp>
#include <fetchpp/json_body.hpp>
#include <fetchpp/request.hpp>
#include <fetchpp/url.hpp>

#include <iostream>
#include <sstream>

#include <catch2/catch.hpp>

class ContentTypeMatcher : public Catch::MatcherBase<std::string_view>
{
  fetchpp::content_type ref_ct;

public:
  template <typename... Args>
  ContentTypeMatcher(Args&&... args)
    : ref_ct(fetchpp::content_type(std::forward<Args>(args)...))
  {
  }

  [[nodiscard]] bool match(std::string_view const& i) const noexcept override
  try
  {
    return ref_ct == fetchpp::content_type::parse(i);
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
  using fetchpp::content_type;
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
  using namespace fetchpp::http;
  {
    auto url = fetchpp::url::parse("http://domain.com:2121/target");
    auto request = make_request(verb::get, url);
    REQUIRE(request[field::host] == "domain.com:2121");
  }
  {
    auto url = fetchpp::url::parse("http://domain.com/target");
    url.port(4242);
    auto request = make_request(verb::get, url);
    REQUIRE(request[field::host] == "domain.com:4242");
  }
}

TEST_CASE("request content_type", "[request]")
{
  auto const req =
      fetchpp::make_request<fetchpp::request<fetchpp::http::json_body>>(
          fetchpp::http::verb::get, fetchpp::url::parse("http://toto.com"), {});
  REQUIRE(req[fetchpp::http::field::content_type] == "application/json");
}
