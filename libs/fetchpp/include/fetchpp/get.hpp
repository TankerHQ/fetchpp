#pragma once

#include <fetchpp/fetch.hpp>
#include <fetchpp/http/response.hpp>

#include <fetchpp/http/headers.hpp>

namespace fetchpp
{
template <typename CompletionToken>
auto async_get(net::executor ex,
               std::string_view url_str,
               http::headers fields,
               CompletionToken&& token)
{
  auto request = http::request(http::verb::get, http::url(url_str));
  for (auto const& field : fields)
    request.insert(field.field, field.field_name, field.value);
  return async_fetch(
      ex, std::move(request), std::forward<CompletionToken>(token));
}

template <typename CompletionToken>
auto async_get(net::executor ex,
               std::string_view url_str,
               CompletionToken&& token)
{
  return async_get(ex, url_str, {}, std::forward<CompletionToken>(token));
}

}
