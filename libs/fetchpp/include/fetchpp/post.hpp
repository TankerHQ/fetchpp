#pragma once

#include <fetchpp/fetch.hpp>

#include <fetchpp/http/headers.hpp>

#include <boost/asio/buffer.hpp>

#include <fetchpp/alias/net.hpp>
#include <fetchpp/alias/strings.hpp>

namespace fetchpp
{
template <typename CompletionToken>
auto async_post(net::executor ex,
                std::string_view url_str,
                net::const_buffer body,
                http::headers fields,
                CompletionToken&& token)
{
  auto request = http::request(http::verb::post, http::url(url_str));
  request.content(body);
  for (auto const& field : fields)
    request.insert(field.field, field.field_name, field.value);
  return async_fetch(
      ex, std::move(request), std::forward<CompletionToken>(token));
}

template <typename CompletionToken>
auto async_post(net::executor ex,
                string_view url_str,
                net::const_buffer body,
                CompletionToken&& token)
{
  return async_post(
      ex, url_str, std::move(body), {}, std::forward<CompletionToken>(token));
}
}
