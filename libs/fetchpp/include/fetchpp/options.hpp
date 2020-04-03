#pragma once

#include <fetchpp/cache_mode.hpp>
#include <fetchpp/field_arg.hpp>
#include <fetchpp/redirect_handling.hpp>

#include <vector>

namespace fetchpp
{
struct options
{
  options() = default;
  std::size_t version = 11;
  cache_mode cache = cache_mode::no_store;
  redirect_handling redirect = redirect_handling::manual;
  connection persistence = connection::close;
};
}
