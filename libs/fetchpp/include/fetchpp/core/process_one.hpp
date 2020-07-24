#pragma once

#include <fetchpp/core/transport_traits.hpp>

#include <fetchpp/core/detail/coroutine.hpp>

#include <boost/asio/compose.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>

#include <fetchpp/alias/beast.hpp>
#include <fetchpp/alias/error_code.hpp>
#include <fetchpp/alias/http.hpp>
#include <fetchpp/alias/net.hpp>

namespace fetchpp
{
namespace detail
{
template <typename AsyncStream,
          typename Buffer,
          typename Response,
          typename CompletionToken,
          bool = Response::is_request::value == false>
auto run_async_read(AsyncStream& stream,
                    Buffer& buffer,
                    Response& response,
                    CompletionToken&& token)
{
  return http::async_read(stream, buffer, response, std::move(token));
}

template <typename AsyncStream,
          typename Request,
          typename CompletionToken,
          bool = Request::is_request::value == true>
auto run_async_write(AsyncStream& stream,
                     Request& request,
                     CompletionToken&& token)
{
  return http::async_write(stream, request, std::move(token));
}

template <typename AsyncStream,
          typename Request,
          typename Response,
          typename Buffer>
struct process_one_stream_op
{
  AsyncStream& stream;
  Request& req;
  Response& res;
  Buffer& buffer;
  net::coroutine coro = net::coroutine{};

  template <typename Self>
  void operator()(Self& self, error_code ec = error_code{}, std::size_t = 0)
  {
    if (ec)
    {
      self.complete(ec);
      return;
    }

    FETCHPP_REENTER(coro)
    {
      FETCHPP_YIELD run_async_write(stream, req, std::move(self));
      FETCHPP_YIELD run_async_read(stream, buffer, res, std::move(self));
      self.complete(ec);
    }
  }
};
template <typename AsyncTransport, typename Request, typename Response>
struct process_one_transport_op
{
  AsyncTransport& transport_;
  Request& req;
  Response& res;

  net::coroutine coro = net::coroutine{};
  template <typename Self>
  void operator()(Self& self, error_code ec = error_code{}, std::size_t = 0)
  {
    if (ec)
    {
      self.complete(ec);
      return;
    }
    FETCHPP_REENTER(coro)
    {
      transport_.setup_timer();
      FETCHPP_YIELD async_process_one(transport_.next_layer(),
                                      transport_.buffer(),
                                      req,
                                      res,
                                      std::move(self));
      transport_.cancel_timer();
      self.complete(ec);
    }
  }
};
}

template <typename AsyncStream,
          typename Buffer,
          typename Request,
          typename Response,
          typename CompletionToken>
auto async_process_one(AsyncStream& stream,
                       Buffer& buffer,
                       Request& request,
                       Response& response,
                       CompletionToken&& token) ->
    typename net::async_result<typename std::decay_t<CompletionToken>,
                               void(error_code)>::return_type

{
  static_assert(beast::is_async_stream<AsyncStream>::value,
                "AsyncStream type requirements not met");
  static_assert(net::is_dynamic_buffer<Buffer>::value,
                "Buffer type requirements not met");

  return net::async_compose<CompletionToken, void(error_code)>(
      detail::process_one_stream_op<AsyncStream, Request, Response, Buffer>{
          stream, request, response, buffer},
      token,
      stream);
}

template <typename AsyncTransport,
          typename Request,
          typename Response,
          typename CompletionToken>
auto async_process_one(AsyncTransport& transport,
                       Request& request,
                       Response& response,
                       CompletionToken&& token) ->
    typename net::async_result<typename std::decay_t<CompletionToken>,
                               void(error_code)>::return_type

{
  static_assert(is_async_transport<AsyncTransport>::value,
                "AsyncTransport type requirements not met");
  return net::async_compose<CompletionToken, void(error_code)>(
      detail::process_one_transport_op<AsyncTransport, Request, Response>{
          transport, request, response},
      token,
      transport);
}
}
