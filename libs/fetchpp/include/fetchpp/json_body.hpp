#pragma once

#include <fetchpp/detail/json_body.hpp>

#include <optional>
#include <string_view>

namespace fetchpp::http
{
struct json_body
{
  using value_type = detail::json_wrapper;
  using reader = detail::json_reader;
  using writer = detail::json_writer;
};
}
