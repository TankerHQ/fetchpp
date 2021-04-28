#pragma once

#include <boost/beast/core/basic_stream.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/core/stream_traits.hpp>

#include <fetchpp/alias/error_code.hpp>

namespace fetchpp::detail
{
template <typename AsyncStream>
void do_cancel(AsyncStream&& stream)
{
  error_code ec = {};
  // we deliberately ignore the error here,
  // there is not much we can do about it.
  beast::get_lowest_layer(stream).socket().cancel(ec);
  assert(!ec);
}
}
