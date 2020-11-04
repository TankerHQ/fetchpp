#pragma once

#include <fetchpp/http/authorization.hpp>
#include <fetchpp/http/content_type.hpp>

#include <boost/beast/core/buffers_cat.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/http/basic_dynamic_body.hpp>
#include <boost/beast/http/message.hpp>

#include <boost/asio/buffer.hpp>
#include <boost/beast/http/fields.hpp>

#include <nlohmann/json.hpp>

#include <fetchpp/alias/http.hpp>
#include <fetchpp/alias/net.hpp>

#include <charconv>
#include <optional>

namespace fetchpp::http
{
namespace detail
{
template <bool isRequest, typename DynamicBuffer>
using dynamic_body_message =
    beast::http::message<isRequest,
                         beast::http::basic_dynamic_body<DynamicBuffer>>;
}

template <bool isRequest, typename DynamicBuffer>
class message : public detail::dynamic_body_message<isRequest, DynamicBuffer>
{

public:
  using base_t = detail::dynamic_body_message<isRequest, DynamicBuffer>;

protected:
  using base_t::base_t;

public:
  message() = default;
  message(message const&) = default;
  message(message&&) = default;
  message(base_t&& other);
  message(base_t const& other);

  message& operator=(base_t&& other);
  message& operator=(base_t const& other);
  message& operator=(message const&) = default;
  message& operator=(message&&) = default;

  bool has_content_type() const;
  std::optional<http::content_type> content_type() const;
  void content_type(http::content_type ctype);

  std::optional<std::size_t> content_length() const;

  using base_t::set;
  void set(http::authorization::methods const& m);

  bool is_json() const;
  bool is_text() const;
  bool is_content() const;
  bool is_content_type(std::string_view type) const;

  auto json() const -> nlohmann::json;
  auto text() const -> std::string;
  auto content() const -> beast::buffers_cat_view<
      typename base_t::body_type::value_type::const_buffers_type>;

  void content(std::string_view value);
  void content(net::const_buffer buffer);
};

//==========

template <bool isRequest, typename DynamicBuffer>
message<isRequest, DynamicBuffer>::message(base_t&& other)
  : base_t(std::move(other))
{
}

template <bool isRequest, typename DynamicBuffer>
message<isRequest, DynamicBuffer>::message(base_t const& other) : base_t(other)
{
}

template <bool isRequest, typename DynamicBuffer>
message<isRequest, DynamicBuffer>& message<isRequest, DynamicBuffer>::operator=(
    base_t&& other)
{
  this->base_t::operator=(std::move(other));
  return *this;
}

template <bool isRequest, typename DynamicBuffer>
message<isRequest, DynamicBuffer>& message<isRequest, DynamicBuffer>::operator=(
    base_t const& other)
{
  this->base_t::operator=(other);
  return *this;
}

template <bool isRequest, typename DynamicBuffer>
std::optional<std::size_t> message<isRequest, DynamicBuffer>::content_length()
    const
{
  if (!this->has_content_length())
    return std::nullopt;
  auto view = this->at(beast::http::field::content_length);
  std::size_t result;
  std::from_chars(view.data(), view.data() + view.size(), result);
  return result;
}

template <bool isRequest, typename DynamicBuffer>
void message<isRequest, DynamicBuffer>::set(
    http::authorization::methods const& m)
{
  this->set(fetchpp::http::field::authorization, m);
}

template <bool isRequest, typename DynamicBuffer>
bool message<isRequest, DynamicBuffer>::has_content_type() const
{
  return this->find(http::field::content_type) != this->end();
}

template <bool isRequest, typename DynamicBuffer>
std::optional<http::content_type>
message<isRequest, DynamicBuffer>::content_type() const
{
  if (!this->has_content_type())
    return std::nullopt;
  return http::content_type::parse(this->at(http::field::content_type));
}

template <bool isRequest, typename DynamicBuffer>
void message<isRequest, DynamicBuffer>::content_type(http::content_type ctype)
{
  this->insert(http::field::content_type, ctype);
}

template <bool isRequest, typename DynamicBuffer>
bool message<isRequest, DynamicBuffer>::is_content_type(
    std::string_view type) const
{
  if (auto const ct = this->content_type())
    if (ct->type() == type)
      return true;
  return false;
}

template <bool isRequest, typename DynamicBuffer>
bool message<isRequest, DynamicBuffer>::is_json() const
{
  return this->is_content_type("application/json");
}

template <bool isRequest, typename DynamicBuffer>
bool message<isRequest, DynamicBuffer>::is_text() const
{
  return this->is_content_type("text/plain");
}

template <bool isRequest, typename DynamicBuffer>
bool message<isRequest, DynamicBuffer>::is_content() const
{
  return this->is_content_type("application/octet-stream") ||
         (!is_json() && !is_text());
}

template <bool isRequest, typename DynamicBuffer>
auto message<isRequest, DynamicBuffer>::text() const -> std::string
{
  return beast::buffers_to_string(this->content());
}

template <bool isRequest, typename DynamicBuffer>
auto message<isRequest, DynamicBuffer>::content() const
    -> beast::buffers_cat_view<
        typename base_t::body_type::value_type::const_buffers_type>
{
  return beast::buffers_cat(this->body().cdata());
}

template <bool isRequest, typename DynamicBuffer>
auto message<isRequest, DynamicBuffer>::json() const -> nlohmann::json
{
  return nlohmann::json::parse(this->text());
}

template <bool isRequest, typename DynamicBuffer>
void message<isRequest, DynamicBuffer>::content(std::string_view value)
{
  auto dest = this->body().prepare(value.size());
  this->body().commit(net::buffer_copy(dest, net::buffer(value)));
  this->insert(http::field::content_type, "text/plain");
  this->prepare_payload();
}

template <bool isRequest, typename DynamicBuffer>
void message<isRequest, DynamicBuffer>::content(net::const_buffer buffer)
{
  auto dest = this->body().prepare(buffer.size());
  this->body().commit(net::buffer_copy(dest, buffer));
  this->insert(http::field::content_type, "application/octet-stream");
  this->prepare_payload();
}

}
