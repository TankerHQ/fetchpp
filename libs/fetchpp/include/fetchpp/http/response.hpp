#pragma once

#include <fetchpp/http/content_type.hpp>
#include <fetchpp/http/simple_response.hpp>

#include <boost/beast/http/vector_body.hpp>

#include <fetchpp/alias/http.hpp>
#include <fetchpp/alias/net.hpp>

#include <nlohmann/json.hpp>

#include <optional>

namespace fetchpp::http
{
class response : public simple_response<beast::http::vector_body<std::uint8_t>>
{
public:
  using base_t = simple_response<beast::http::vector_body<std::uint8_t>>;
  using text_t = std::string;
  using json_type = nlohmann::json;
  using content_t = typename base_t::body_type::value_type;

  response() = default;
  response(response&&) = default;
  response(response const&) = default;
  response& operator=(response const&) = default;
  using base_t::operator=;

  bool has_content_type() const;
  http::content_type const& content_type() const;

  bool is_json() const;
  bool is_text() const;
  bool is_content() const;

  json_type json() const;
  text_t text() const;
  content_t content() const;

private:
  mutable std::optional<http::content_type> _content_type;
};
}
