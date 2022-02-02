#pragma once

#include <fetchpp/core/basic_transport.hpp>
#include <fetchpp/core/detail/coroutine.hpp>

#include <boost/asio/compose.hpp>
#include <boost/beast/core/bind_handler.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/core/stream_traits.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>

#include <fetchpp/alias/beast.hpp>
#include <fetchpp/alias/error_code.hpp>
#include <fetchpp/alias/tcp.hpp>

namespace fetchpp::detail
{
template <typename AsyncSSLStream>
struct async_ssl_close_op
{
  static_assert(beast::is_async_stream<AsyncSSLStream>::value,
                "AsyncStream type requirements not met");
  AsyncSSLStream& stream_;
  net::coroutine coro_ = {};

  template <typename Self>
  void operator()(Self& self, error_code ec = {})
  {
    FETCHPP_REENTER(coro_)
    {
      FETCHPP_YIELD stream_.async_shutdown(std::move(self));
      if (ec == net::error::eof)
        // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
        ec = {};
      else if (ec == net::ssl::error::stream_truncated)
        // lots of implementation do not respect the graceful shutdown
        ec = {};
      else if (ec == net::ssl::error::unspecified_system_error)
        // we most probably canceled our stream, which is now in an
        // inconsisent state. Forsake all etiquette, we want out anyway.
        ec = {};
      else if (ec == net::error::operation_aborted)
        ec = {};
      beast::close_socket(beast::get_lowest_layer(stream_));
      self.complete(ec);
    }
  }
};
template <typename NextLayer>
async_ssl_close_op(beast::ssl_stream<NextLayer>&)
    -> async_ssl_close_op<beast::ssl_stream<NextLayer>>;

}
