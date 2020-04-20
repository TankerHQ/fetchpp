#pragma once

#include <fetchpp/request.hpp>

#include <boost/asio/compose.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>

#include <fetchpp/detail/coroutine.hpp>

#include <fetchpp/alias/beast.hpp>
#include <fetchpp/alias/error_code.hpp>
#include <fetchpp/alias/http.hpp>
#include <fetchpp/alias/net.hpp>

namespace fetchpp
{
namespace detail
{
template <typename AsyncStream,
          typename Request,
          typename Response,
          typename Buffer>
struct process_one_composer
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
      FETCHPP_YIELD http::async_write(stream, req, std::move(self));
      FETCHPP_YIELD http::async_read(stream, buffer, res, std::move(self));
      self.complete(ec);
    }
  }
};
}

template <typename CompletionToken,
          typename AsyncStream,
          typename Request,
          typename Response,
          typename Buffer>
auto async_process_one(AsyncStream& stream,
                       Request& request,
                       Response& response,
                       Buffer& buffer,
                       CompletionToken&& token) ->
    typename net::async_result<typename std::decay_t<CompletionToken>,
                               void(error_code)>::return_type

{
  static_assert(beast::is_async_stream<AsyncStream>::value,
                "AsyncStream type requirements not met");
  static_assert(net::is_dynamic_buffer<Buffer>::value,
                "Buffer type requirements not met");

  return net::async_compose<CompletionToken, void(error_code)>(
      detail::process_one_composer<AsyncStream, Request, Response, Buffer>{
          stream, request, response, buffer},
      token,
      stream);
}
}
