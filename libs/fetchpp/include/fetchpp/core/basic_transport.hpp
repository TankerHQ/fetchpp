#pragma once

#include <boost/asio/buffer.hpp>
#include <boost/beast/core/stream_traits.hpp>

#include <fetchpp/alias/beast.hpp>
#include <fetchpp/alias/net.hpp>

#include <string>
#include <string_view>

namespace fetchpp
{
namespace detail
{
class basic_transport_base
{
public:
  std::string const& domain() const;
  std::uint16_t port() const;
  std::string_view host() const;

  basic_transport_base(std::string domain, std::uint16_t port);

private:
  std::string domain_;
  std::uint16_t port_;
};
}

template <typename AsyncStream, typename DynamicBuffer>
class basic_async_transport : public detail::basic_transport_base
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
  basic_async_transport(std::string domain,
                        std::uint16_t port,
                        DynamicBuffer buffer,
                        Args&&... args)
    : basic_transport_base(std::move(domain), port),
      buffer_(std::move(buffer), stream_(std::forward<Args>(args)...))
  {
  }

  template <typename... Args>
  basic_async_transport(std::string domain, std::uint16_t port, Args&&... args)
    : basic_transport_base(std::move(domain), port),
      buffer_(buffer_type{}),
      stream_(std::forward<Args>(args)...)
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
