#pragma once

#include <fetchpp/core/endpoint.hpp>

#include <boost/asio/buffer.hpp>
#include <boost/beast/core/stream_traits.hpp>

#include <fetchpp/alias/beast.hpp>
#include <fetchpp/alias/net.hpp>

namespace fetchpp
{
template <typename AsyncStream, typename DynamicBuffer>
class basic_async_transport
{
  static_assert(beast::is_async_stream<AsyncStream>::value,
                "AsyncStream type requirements not met");
  static_assert(net::is_dynamic_buffer<DynamicBuffer>::value,
                "DynamicBuffer type requirements not met");

public:
  using buffer_type = DynamicBuffer;
  using next_layer_type = AsyncStream;

  using executor_type = typename next_layer_type::executor_type;

  template <typename... Args>
  basic_async_transport(DynamicBuffer buffer, Args&&... args)
    : buffer_(std::move(buffer)), stream_(std::forward<Args>(args)...)
  {
  }

  template <typename... Args>
  basic_async_transport(Args&&... args)
    : basic_async_transport(buffer_type{}, std::forward<Args>(args)...)
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
    return do_async_connect(
        endpoint, *this, std::forward<CompletionToken>(token));
  }

  next_layer_type& next_layer()
  {
    return stream_;
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

private:
  buffer_type buffer_;
  next_layer_type stream_;
};

template <class T>
using is_async_transport = std::integral_constant<
    bool,
    beast::is_async_stream<T>::value &&
        net::is_dynamic_buffer<typename T::buffer_type>::value>;

}
