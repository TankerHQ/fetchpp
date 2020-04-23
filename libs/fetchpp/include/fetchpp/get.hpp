#pragma once

#include <fetchpp/fetch.hpp>
#include <fetchpp/request.hpp>

namespace fetchpp
{
template <typename GetHandler>
auto async_get(net::io_context& ioc,
               std::string const& url_str,
               headers fields,
               GetHandler&& handler)
{
  auto request = make_request(http::verb::get, url::parse(url_str), {});
  for (auto const& field : fields)
    request.insert(field.field, field.field_name, field.value);
  return async_fetch(ioc, request, handler);
}

}
