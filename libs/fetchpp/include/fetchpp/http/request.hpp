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
class simple_request : public message<true, DynamicBuffer>
{
  using base_t = message<true, DynamicBuffer>;

public:
  simple_request(http::verb verb, url uri);

  url const& uri() const;

private:
  url _uri;
};

// =================

template <typename DynamicBuffer>
simple_request<DynamicBuffer>::simple_request(http::verb verb, url uri)
  : base_t(verb, uri.target(), 11), _uri(std::move(uri))
{
  detail::set_options(_uri.host(), *this);
}

template <typename DynamicBuffer>
url const& simple_request<DynamicBuffer>::uri() const
{
  return _uri;
}

using request = simple_request<beast::multi_buffer>;
}
