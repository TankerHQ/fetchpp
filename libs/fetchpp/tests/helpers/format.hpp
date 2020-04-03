#pragma once

#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/core/multi_buffer.hpp>
#include <boost/utility/string_view.hpp>

#include <fmt/core.h>

namespace boost
{
fmt::string_view to_string_view(boost::string_view const& v);
}

namespace fmt
{
template <>
struct formatter<boost::beast::multi_buffer>
{
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(boost::beast::multi_buffer const& b, FormatContext& ctx)
      -> decltype(ctx.out());
};
extern template struct formatter<boost::beast::multi_buffer>;
}
