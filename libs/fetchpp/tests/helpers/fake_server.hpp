#include "peer_session.hpp"

#include <boost/asio/basic_socket_acceptor.hpp>
#include <boost/asio/compose.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>

namespace test::helpers
{
namespace bb = boost::beast;
namespace net = boost::asio;

class fake_server;

namespace detail
{
struct acceptor_base_op
{
  using acceptor_type = net::basic_socket_acceptor<net::ip::tcp, net::executor>;
  acceptor_type& acceptor_;

  template <typename Self>
  void operator()(Self& self)
  {
    acceptor_.async_accept(std::move(self));
  }
};

template <typename AsyncStream>
struct acceptor_op : acceptor_base_op
{
  using acceptor_base_op::operator();
  template <typename Self>
  void operator()(Self& self, bb::error_code ec, net::ip::tcp::socket socket)
  {
    self.complete(
        ec,
        std::move(peer_session<AsyncStream>(AsyncStream(std::move(socket)))));
  }
};

template <typename AsyncStream>
struct acceptor_ssl_op : acceptor_base_op
{
  net::ssl::context& context_;
  std::unique_ptr<AsyncStream> stream_ = {};

  using acceptor_base_op::operator();
  template <typename Self>
  void operator()(Self& self, bb::error_code ec, net::ip::tcp::socket socket)
  {
    if (ec)
    {
      self.complete(ec, {});
      return;
    }
    stream_ = std::make_unique<AsyncStream>(std::move(socket), context_);
    stream_->async_handshake(AsyncStream::handshake_type::server,
                             std::move(self));
  }

  template <typename Self>
  void operator()(Self& self, bb::error_code ec)
  {
    if (ec)
    {
      self.complete(ec, {});
      return;
    }
    self.complete(
        ec,
        std::move(peer_session<AsyncStream>(std::move(*stream_.release()))));
  }
};
}

class fake_server
{
  using executor_type = net::executor;
  using acceptor_type = net::basic_socket_acceptor<net::ip::tcp, executor_type>;

private:
  acceptor_type acceptor_;

public:
  fake_server(executor_type ex)
    : acceptor_(ex,
                typename acceptor_type::endpoint_type(
                    net::ip::make_address_v4("127.0.0.1"), 2142))
  {
  }

  auto get_executor()
  {
    return acceptor_.get_executor();
  }

  template <typename AsyncStream = bb::tcp_stream, typename CompletionToken>
  auto async_accept(CompletionToken&& token)
  {
    return net::async_compose<CompletionToken,
                              void(bb::error_code, peer_session<AsyncStream>)>(
        detail::acceptor_op<AsyncStream>{acceptor_},
        std::forward<CompletionToken>(token),
        acceptor_);
  }

  template <typename AsyncStream = bb::ssl_stream<bb::tcp_stream>,
            typename CompletionToken>
  auto async_accept(net::ssl::context& context, CompletionToken&& token)
  {
    return net::async_compose<CompletionToken,
                              void(bb::error_code, peer_session<AsyncStream>)>(
        detail::acceptor_ssl_op<AsyncStream>{acceptor_, context},
        token,
        acceptor_);
  }

  auto local_endpoint()
  {
    return acceptor_.local_endpoint();
  }
};
}
