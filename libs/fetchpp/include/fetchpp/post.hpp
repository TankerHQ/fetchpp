#pragma once

#include <fetchpp/fetch.hpp>

#include <fetchpp/http/headers.hpp>

namespace fetchpp
{
template <typename Request, typename CompletionToken>
auto async_post(net::executor ex,
                std::string const& url_str,
                typename Request::body_type::value_type data,
                http::headers fields,
                CompletionToken&& token)
{
  auto request = http::make_request<Request>(
      http::verb::post, http::url::parse(url_str), {}, std::move(data));
  for (auto const& field : fields)
    request.insert(field.field, field.field_name, field.value);
  return async_fetch(
      ex, std::move(request), std::forward<CompletionToken>(token));
}

template <typename Request, typename CompletionToken>
auto async_post(net::executor ex,
                std::string_view url_str,
                typename Request::body_type::value_type data,
                CompletionToken&& token)
{
  return async_post(
      ex, url_str, std::move(data), {}, std::forward<CompletionToken>(token));
}
}
