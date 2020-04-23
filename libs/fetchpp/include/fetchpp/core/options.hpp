#pragma once

#include <fetchpp/core/cache_mode.hpp>
#include <fetchpp/core/redirect_handling.hpp>

#include <vector>

namespace fetchpp
{
enum class http_version : std::size_t
{
  v10 = 10,
  v11 = 11,
};

struct options
{
  options() = default;

public:
  options& set(http_version hv)
  {
    this->version = hv;
    return *this;
  }

  options& set(cache_mode c)
  {
    this->cache = c;
    return *this;
  }

  options& set(redirect_handling r)
  {
    this->redirect = r;
    return *this;
  }

  http_version version = http_version::v11;
  cache_mode cache = cache_mode::no_store;
  redirect_handling redirect = redirect_handling::manual;
};
}
