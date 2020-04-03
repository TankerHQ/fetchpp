#pragma once

#include <string>

namespace fetchpp
{
enum class cache_mode : int
{
  // max_age,
  // min_fresh,
  max_stale,
  no_cache,
  no_store,
  no_transform,
  only_if_cached,
};
std::string to_string(cache_mode);

enum class connection : int
{
  keep_alive,
  close,
  upgrade,
};

std::string to_string(connection);
}
