#include <fetchpp/response.hpp>

#include <boost/asio/buffers_iterator.hpp>
#include <boost/beast/core/buffers_to_string.hpp>

namespace fetchpp
{

bool response::has_content_type() const
{
  return _content_type || find(http::field::content_type) != end();
}

// FIXME errors!
fetchpp::content_type const& response::content_type() const
{
  if (_content_type)
    return *_content_type;
  if (find(http::field::content_type) != end())
    _content_type = fetchpp::content_type::parse(at(http::field::content_type));
  return _content_type.value();
}

bool response::is_json() const
{
  if (!has_content_type())
    return false;
  if (auto const ct = content_type(); ct.type() == "application/json")
    return true;
  return false;
}

bool response::is_text() const
{
  if (!has_content_type())
    return false;
  if (auto const ct = content_type(); ct.type() == "text/plain")
    return true;
  return false;
}

bool response::is_content() const
{
  if (!has_content_type())
    return false;
  if (auto const ct = content_type();
      ct.type() == "applicatioj/octet-stream" || (!is_json() && !is_text()))
    return true;
  return false;
}

auto response::json() const -> json_type
{
  return nlohmann::json::parse(body().begin(), body().end());
}

auto response::text() const -> std::string
{
  return {body().begin(), body().end()};
}

auto response::content() const -> content_t
{
  return body();
}

}
