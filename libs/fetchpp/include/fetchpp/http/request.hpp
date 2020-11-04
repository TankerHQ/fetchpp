#pragma once

#include <fetchpp/http/detail/request.hpp>
#include <fetchpp/http/message.hpp>
#include <fetchpp/http/url.hpp>

#include <boost/beast/core/multi_buffer.hpp>
#include <boost/beast/http/verb.hpp>

#include <fetchpp/alias/http.hpp>

#include <optional>

namespace fetchpp::http
{
template <typename DynamicBuffer>
class basic_request : public message<true, DynamicBuffer>
{
  using base_t = message<true, DynamicBuffer>;

public:
  basic_request(http::verb verb, url uri);

  url const& uri() const;
  void accept(std::string_view ct);
  void accept(http::content_type const& ct);

private:
  url _uri;
};

// =================

template <typename DynamicBuffer>
basic_request<DynamicBuffer>::basic_request(http::verb verb, url uri)
  : base_t(verb, uri.target(), 11), _uri(std::move(uri))
{
  detail::set_options(_uri.host(), *this);
}

template <typename DynamicBuffer>
url const& basic_request<DynamicBuffer>::uri() const
{
  return _uri;
}

template <typename DynamicBuffer>
void basic_request<DynamicBuffer>::accept(std::string_view ct)
{
  this->set(http::field::accept, ct);
}

template <typename DynamicBuffer>
void basic_request<DynamicBuffer>::accept(http::content_type const& ct)
{
  this->set(http::field::accept, ct);
}

using request = basic_request<beast::multi_buffer>;
}
