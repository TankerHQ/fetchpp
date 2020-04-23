#pragma once

#include <fetchpp/fetch.hpp>

#include <fetchpp/http/headers.hpp>

namespace fetchpp
{
template <typename Request, typename GetHandler>
auto async_post(net::io_context& ioc,
                std::string const& url_str,
                typename Request::body_type::value_type data,
                http::headers fields,
                GetHandler&& handler)
{
  auto request = http::make_request<Request>(
      http::verb::post, http::url::parse(url_str), {}, std::move(data));
  for (auto const& field : fields)
    request.insert(field.field, field.field_name, field.value);
  return async_fetch(ioc, std::move(request), handler);
}
}
