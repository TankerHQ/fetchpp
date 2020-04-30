#include <boost/asio/compose.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/core/stream_traits.hpp>

#include <boost/asio/error.hpp>

#include <fetchpp/alias/error_code.hpp>
#include <fetchpp/alias/tcp.hpp>

namespace fetchpp::detail
{
template <typename AsyncStream>
struct connect_composer
{
  AsyncStream& stream;
  std::string_view domain;
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
        // https://www.cloudflare.com/learning/ssl/what-is-sni/
        if (!SSL_set_tlsext_host_name(this->stream.native_handle(),
                                      domain.data()))
          return self.complete(error_code{static_cast<int>(::ERR_get_error()),
                                          net::error::get_ssl_category()});
        beast::get_lowest_layer(this->stream)
            .async_connect(this->results, std::move(self));
        return;
      case connecting:
      case handshaking:
        break;
      }
    }
    self.complete(ec);
  }

  template <typename Self>
  void operator()(Self& self,
                  error_code ec,
                  tcp::resolver::results_type::endpoint_type)
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
