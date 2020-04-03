#pragma once

#include <boost/asio/compose.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>

#include <fetchpp/alias/beast.hpp>
#include <fetchpp/alias/error_code.hpp>
#include <fetchpp/alias/net.hpp>
#include <fetchpp/alias/tcp.hpp>

namespace fetchpp
{
namespace detail
{
template <typename AsyncStream>
struct connect_composer
{
  AsyncStream& stream;
  tcp::resolver::results_type& results;
  enum
  {
    starting,
    connecting,
    handshaking,
  } _status = starting;

  template <typename Self>
  void operator()(Self& self, error_code ec = error_code{})
  {
    if (!ec)
    {
      switch (_status)
      {
      case starting:
        _status = connecting;
        beast::get_lowest_layer(this->stream)
            .async_connect(this->results, std::move(self));
        return;
      case handshaking:
        break;
      }
    }
    self.complete(ec);
  }

  template <typename Self>
  void operator()(Self& self,
                  error_code ec,
                  tcp::resolver::results_type::endpoint_type endpoint)
  {
    if (ec)
    {
      self.complete(ec);
      return;
    }
    _status = handshaking;
    this->stream.async_handshake(net::ssl::stream_base::client,
                                 std::move(self));
  }
};
}

template <typename CompletionToken, typename AsyncStream>
auto async_connect(AsyncStream& stream,
                   tcp::resolver::results_type& results,
                   CompletionToken&& token) ->
    typename net::async_result<typename std::decay_t<CompletionToken>,
                               void(error_code)>::return_type
{
  return net::async_compose<CompletionToken, void(error_code)>(
      detail::connect_composer<AsyncStream>{stream, results}, token, stream);
}
}
