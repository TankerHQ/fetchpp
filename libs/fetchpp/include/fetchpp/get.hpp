#pragma once

#include <fetchpp/fetch.hpp>
#include <fetchpp/http/response.hpp>

#include <fetchpp/http/headers.hpp>

#include <fetchpp/alias/strings.hpp>

#include <string_view>

namespace fetchpp
{
template <typename CompletionToken>
auto async_get(net::any_io_executor ex,
               string_view url_str,
               http::headers fields,
               CompletionToken&& token)
{
  auto request = http::request(
      http::verb::get,
      http::url(std::string_view{url_str.data(), url_str.size()}));
  for (auto const& field : fields)
    request.insert(field.field, field.field_name, field.value);
  return async_fetch(
      ex, std::move(request), std::forward<CompletionToken>(token));
}

template <typename CompletionToken>
auto async_get(net::any_io_executor ex, string_view url_str, CompletionToken&& token)
{
  return async_get(ex, url_str, {}, std::forward<CompletionToken>(token));
}

}
