#pragma once

#include <fetchpp/core/endpoint.hpp>
#include <fetchpp/http/url.hpp>

namespace fetchpp::detail
{
template <bool IsSecure = true>
basic_endpoint<IsSecure> to_endpoint(http::url const& url)
{
  auto const port = http::safe_port(url);
  if constexpr (IsSecure)
  {
    if (url.protocol() != "https:")
      throw std::domain_error("invalid url for secure endpoint");
    return secure_endpoint(url.hostname(), port);
  }
  else
    return plain_endpoint(url.hostname(), port);
}
}