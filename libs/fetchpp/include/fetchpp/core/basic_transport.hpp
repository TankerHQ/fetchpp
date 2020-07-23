#pragma once

#include <fetchpp/core/endpoint.hpp>

#include <boost/asio/buffer.hpp>
#include <boost/asio/compose.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/stream_traits.hpp>

#include <fetchpp/core/detail/coroutine.hpp>

#include <fetchpp/alias/beast.hpp>
#include <fetchpp/alias/error_code.hpp>
#include <fetchpp/alias/net.hpp>
#include <fetchpp/alias/tcp.hpp>

#include <chrono>

namespace fetchpp
{
namespace detail
{
template <typename AsyncTransport>
struct async_basic_connect_op
{
  detail::base_endpoint& endpoint_;
  AsyncTransport& transport_;
  net::coroutine coro_ = {};

  template <typename Self>
  void operator()(Self& self,
                  error_code ec = {},
                  tcp::resolver::results_type results = {})
  {
    if (ec)
    {
      transport_.cancel_timer();
      self.complete(ec);
      return;
    }
    if (!transport_.running_)
    {
      transport_.cancel_timer();
      self.complete(net::error::operation_aborted);
      return;
    }
    FETCHPP_REENTER(coro_)
    {
      transport_.set_running(true);
      transport_.setup_timer();
      FETCHPP_YIELD transport_.resolver_.async_resolve(
          endpoint_.domain(),
          std::to_string(endpoint_.port()),
          net::ip::resolver_base::numeric_service,
          std::move(self));
      FETCHPP_YIELD do_async_connect(
          transport_, endpoint_.domain(), std::move(results), std::move(self));
      transport_.cancel_timer();
      self.complete(ec);
    }
  }
};
template <typename AsyncStream>
async_basic_connect_op(detail::base_endpoint&, AsyncStream&)
    ->async_basic_connect_op<AsyncStream>;
}

template <typename AsyncStream, typename DynamicBuffer>
class basic_async_transport
{
  static_assert(beast::is_async_stream<AsyncStream>::value,
                "AsyncStream type requirements not met");
  static_assert(net::is_dynamic_buffer<DynamicBuffer>::value,
                "DynamicBuffer type requirements not met");

  template <typename AsyncTransport>
  friend struct detail::async_basic_connect_op;

public:
  using buffer_type = DynamicBuffer;
  using next_layer_type = AsyncStream;
  using executor_type = typename next_layer_type::executor_type;

  template <typename... Args>
  basic_async_transport(DynamicBuffer buffer,
                        std::chrono::nanoseconds timeout,
                        Args&&... args)
    : buffer_(std::move(buffer)),
      stream_(std::forward<Args>(args)...),
      resolver_(stream_.get_executor()),
      timeout_(timeout)
  {
  }

  template <typename... Args>
  basic_async_transport(std::chrono::nanoseconds timeout, Args&&... args)
    : basic_async_transport(buffer_type{}, timeout, std::forward<Args>(args)...)
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
  auto async_connect(detail::base_endpoint& endpoint, CompletionToken&& token)
  {
    return net::async_compose<CompletionToken, void(error_code)>(
        detail::async_basic_connect_op{endpoint, *this}, token, *this);
  }

  template <typename CompletionToken>
  auto async_close(CompletionToken&& token)
  {
    return net::async_compose<CompletionToken, void(error_code)>(
        [this, coro_ = net::coroutine{}](auto& self,
                                         error_code ec = {}) mutable {
          FETCHPP_REENTER(coro_)
          {
            this->setup_timer();
            FETCHPP_YIELD do_async_close(*this, std::move(self));
            this->cancel_timer();
            self.complete(ec);
          }
        },
        token,
        *this);
  }

  next_layer_type& next_layer()
  {
    return stream_;
  }

  next_layer_type const& next_layer() const
  {
    return stream_;
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
    get_lowest_layer(stream_).expires_after(timeout_);
  }

  void cancel_timer()
  {
    get_lowest_layer(stream_).expires_never();
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
  buffer_type buffer_;
  next_layer_type stream_;
  tcp::resolver resolver_;
  std::chrono::nanoseconds timeout_;
  bool running_ = true;
};

template <class T>
using is_async_transport = std::integral_constant<
    bool,
    beast::is_async_stream<T>::value &&
        net::is_dynamic_buffer<typename T::buffer_type>::value>;
}
