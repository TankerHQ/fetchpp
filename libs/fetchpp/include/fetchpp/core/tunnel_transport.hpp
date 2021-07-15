#pragma once

#include <fetchpp/core/basic_transport.hpp>
#include <fetchpp/core/detail/close_ssl.hpp>
#include <fetchpp/core/endpoint.hpp>
#include <fetchpp/core/process_one.hpp>
#include <fetchpp/http/request.hpp>
#include <fetchpp/http/response.hpp>

#include <boost/asio/compose.hpp>
#include <boost/asio/executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/beast/core/multi_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>

#include <fetchpp/alias/beast.hpp>
#include <fetchpp/alias/error_code.hpp>
#include <fetchpp/alias/net.hpp>
#include <fetchpp/alias/ssl.hpp>
#include <fetchpp/alias/tcp.hpp>

namespace fetchpp
{
class tunnel_async_transport;

namespace detail
{
struct http_messages
{
  using request_t = http::message<true, beast::multi_buffer>;
  http_messages(request_t req) : request(std::move(req)), response()
  {
  }
  request_t request;
  http::response response;
};

template <typename Transport>
struct async_tunnel_connect_op
{
  tunnel_endpoint const& endpoint_;
  Transport& transport_;
  std::unique_ptr<http_messages> transaction_ = {};
  net::coroutine coro_ = {};

  template <typename Self>
  void operator()(Self& self,
                  error_code ec = {},
                  tcp::resolver::results_type::endpoint_type = {})
  {
    if (ec)
    {
      transport_.cancel_timer();
      self.complete(ec);
      return;
    }
    FETCHPP_REENTER(coro_)
    {
      transport_.set_running(true);
      transport_.setup_timer();
      FETCHPP_YIELD transport_.resolver_.async_resolve(
          endpoint_.proxy().domain(),
          std::to_string(endpoint_.proxy().port()),
          net::ip::resolver_base::numeric_service,
          std::move(self));
      beast::get_lowest_layer(transport_)
          .socket()
          .set_option(net::ip::tcp::no_delay{true});
      transaction_ = std::make_unique<http_messages>(http_messages::request_t(
          beast::http::verb::connect, endpoint_.target().host(), 11));
      transaction_->request.set(beast::http::field::host,
                                endpoint_.target().host());
      transaction_->request.set(beast::http::field::proxy_connection,
                                "Keep-Alive");
      FETCHPP_YIELD async_process_one(transport_.next_layer().next_layer(),
                                      transport_.buffer(),
                                      transaction_->request,
                                      transaction_->response,
                                      std::move(self));
      if (transaction_->response.result() != beast::http::status::ok)
      {
        // FIXME: fetchpp::errc::proxy_connection_refused or smth
        self.complete(net::error::connection_refused);
        transport_.cancel_timer();
        return;
      }
      FETCHPP_YIELD transport_.next_layer().async_handshake(
          net::ssl::stream_base::client, std::move(self));
      transport_.cancel_timer();
      self.complete(ec);
    }
  }

  template <typename Self>
  void operator()(Self& self,
                  error_code ec,
                  tcp::resolver::results_type results)
  {
    if (ec)
    {
      self.complete(ec);
      transport_.cancel_timer();
      return;
    }
    // https://www.cloudflare.com/learning/ssl/what-is-sni/
    if (!SSL_set_tlsext_host_name(transport_.next_layer().native_handle(),
                                  endpoint_.target().domain().data()))
      return self.complete(error_code{static_cast<int>(::ERR_get_error()),
                                      net::error::get_ssl_category()});
    beast::get_lowest_layer(transport_).async_connect(results, std::move(self));
  }
};

template <typename Transport>
async_tunnel_connect_op(tunnel_endpoint, Transport&)
    -> async_tunnel_connect_op<Transport>;

template <typename CompletionToken>
auto do_async_close(tunnel_async_transport& ts, CompletionToken&& token);
}

class tunnel_async_transport
{
  template <typename AsyncTransport>
  friend struct detail::async_tunnel_connect_op;

public:
  using buffer_type = beast::multi_buffer;
  using next_layer_type = beast::ssl_stream<beast::tcp_stream>;
  using executor_type = typename next_layer_type::executor_type;
  using next_layer_creator_sig = next_layer_type();
  using next_layer_creator = std::function<next_layer_creator_sig>;

  tunnel_async_transport(net::executor ex,
                         std::chrono::nanoseconds timeout,
                         net::ssl::context& ctx)
    : stream_creator_([ex, &ctx]() { return next_layer_type(ex, ctx); }),
      stream_(std::make_unique<next_layer_type>(stream_creator_())),
      resolver_(next_layer().get_executor()),
      timeout_(timeout)
  {
  }

  template <typename MutableBufferSequence, typename ReadHandler>
  auto async_read_some(MutableBufferSequence const& buffers,
                       ReadHandler&& handler)
  {
    return next_layer().async_read_some(buffers,
                                        std::forward<ReadHandler>(handler));
  }

  template <typename ConstBufferSequence, typename WriteHandler>
  auto async_write_some(ConstBufferSequence const& buffers,
                        WriteHandler&& handler)
  {
    return next_layer().async_write_some(buffers,
                                         std::forward<WriteHandler>(handler));
  }

  template <typename CompletionToken>
  auto async_connect(tunnel_endpoint const& endpoint, CompletionToken&& token)
  {
    return net::async_compose<CompletionToken, void(error_code)>(
        detail::async_tunnel_connect_op{endpoint, *this}, token, *this);
  }

  template <typename CompletionToken>
  auto async_close(CompletionToken&& token)
  {
    return this->async_close(GracefulShutdown::Yes,
                             std::forward<CompletionToken>(token));
  }

  template <typename CompletionToken>
  auto async_close(GracefulShutdown beGraceful, CompletionToken&& token)
  {
    return net::async_compose<CompletionToken, void(error_code)>(
        [this, beGraceful, coro_ = net::coroutine{}](
            auto& self, error_code ec = {}) mutable {
          FETCHPP_REENTER(coro_)
          {
            set_running(false);
            this->setup_timer();
            if (this->is_open())
            {
              detail::do_cancel(*stream_);
              if (beGraceful == GracefulShutdown::Yes)
              {
                FETCHPP_YIELD detail::do_async_close(*this, std::move(self));
              }
            }
            else
            {
              this->resolver().cancel();
              FETCHPP_YIELD net::post(
                  beast::bind_front_handler(std::move(self)));
            }
            this->cancel_timer();
            this->buffer().clear();
            this->reset();
            self.complete(ec);
          }
        },
        token,
        *this);
  }

  next_layer_type& next_layer()
  {
    return *stream_;
  }

  next_layer_type const& next_layer() const
  {
    return *stream_;
  }

  tcp::resolver& resolver()
  {
    return resolver_;
  }

  tcp::resolver const& resolver() const
  {
    return resolver_;
  }

  void setup_timer()
  {
    get_lowest_layer(next_layer()).expires_after(timeout_);
  }

  void cancel_timer()
  {
    get_lowest_layer(next_layer()).expires_never();
  }

  void reset()
  {
    stream_ = std::make_unique<next_layer_type>(stream_creator_());
  }

  bool is_open()
  {
    return beast::get_lowest_layer(next_layer()).socket().is_open();
  }

  auto const& timeout() const
  {
    return timeout_;
  }

  auto get_executor()
  {
    return next_layer().get_executor();
  }

  buffer_type const& buffer() const
  {
    return buffer_;
  }

  buffer_type& buffer()
  {
    return buffer_;
  }

  void set_running(bool r = true)
  {
    running_ = r;
  }

  bool is_running() const
  {
    return running_;
  }

private:
  next_layer_creator stream_creator_;
  buffer_type buffer_;
  std::unique_ptr<next_layer_type> stream_;
  tcp::resolver resolver_;
  std::chrono::nanoseconds timeout_;
  bool running_ = true;
};

namespace detail
{
template <typename CompletionToken>
auto do_async_close(tunnel_async_transport& ts, CompletionToken&& token)
{
  return net::async_compose<CompletionToken, void(error_code)>(
      detail::async_ssl_close_op{ts.next_layer()}, token, ts);
}
}
}
