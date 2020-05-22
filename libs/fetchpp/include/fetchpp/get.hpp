#pragma once

#include <fetchpp/fetch.hpp>

#include <fetchpp/http/headers.hpp>

namespace fetchpp
{
template <typename GetHandler>
auto async_get(net::executor ex,
               std::string const& url_str,
               http::headers fields,
               GetHandler&& handler)
{
  auto request = make_request(http::verb::get, http::url::parse(url_str), {});
  for (auto const& field : fields)
    request.insert(field.field, field.field_name, field.value);
  return async_fetch(ex, request, handler);
}

}
