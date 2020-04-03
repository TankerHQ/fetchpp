#include "format.hpp"

namespace boost
{
fmt::string_view to_string_view(boost::string_view const& v)
{
  return fmt::string_view{v.data(), v.length()};
}
}

namespace fmt
{
template <typename ParseContext>
auto formatter<boost::beast::multi_buffer>::format(
    boost::beast::multi_buffer const& b, ParseContext& ctx)
    -> decltype(ctx.out())
{
  auto const v = boost::beast::buffers_to_string(b.data());
  return format_to(ctx.out(), "{}", v);
}

template struct formatter<boost::beast::multi_buffer>;
};
