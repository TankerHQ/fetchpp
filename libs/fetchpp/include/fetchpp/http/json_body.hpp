#pragma once

#include <fetchpp/http/detail/json_body.hpp>

namespace fetchpp::http
{
struct json_body
{
  using value_type = detail::json_wrapper;
  static std::uint64_t size(value_type const& v);
  using reader = detail::json_reader;
  using writer = detail::json_writer;
};
}
