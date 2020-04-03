#include <fetchpp/cache_mode.hpp>

namespace fetchpp
{
std::string to_string(cache_mode cm)
{
  switch (cm)
  {
  case cache_mode::max_stale:
    return "max_stale";
  case cache_mode::no_cache:
    return "no-cache";
  case cache_mode::no_store:
    return "no-cache";
  case cache_mode::no_transform:
    return "no-transform";
  case cache_mode::only_if_cached:
    return "only-if-cached";
  }
  return "ERROR";
}

std::string to_string(connection con)
{
  switch (con)
  {
  case connection::keep_alive:
    return "keep-alive";
  case connection::close:
    return "close";
  case connection::upgrade:
    return "upgrade";
  }
  return "ERROR";
}
}
