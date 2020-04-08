#pragma once

#include <fetchpp/fetch.hpp>
#include <fetchpp/request.hpp>

namespace fetchpp
{
template <typename Response, typename Request, typename GetHandler>
auto async_post(net::io_context& ioc,
                std::string const& url_str,
                typename Request::body_type::value_type data,
                headers fields,
                GetHandler&& handler)
    -> detail::async_http_result_t<GetHandler, Response>
{
  auto request = make_request<Request>(
      http::verb::post, url::parse(url_str), {}, std::move(data));
  for (auto const& field : fields)
    request.insert(field.field, field.field_name, field.value);
  return async_fetch<Response>(ioc, std::move(request), handler);
}
}
