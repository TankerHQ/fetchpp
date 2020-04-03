#pragma once

#include <fetchpp/request.hpp>

#include <boost/asio/compose.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>

#include <fetchpp/alias/beast.hpp>
#include <fetchpp/alias/error_code.hpp>
#include <fetchpp/alias/http.hpp>
#include <fetchpp/alias/net.hpp>

#include <memory>

namespace fetchpp
{
namespace detail
{
template <typename AsyncStream, typename Request, typename Response>
struct state
{
  AsyncStream& stream;
  Request& req;
  Response& res;
  beast::flat_buffer buffer;

  state(AsyncStream& stream, Request& request, Response& response)
    : stream(stream), req(request), res(response)
  {
  }
};

template <typename StatePtr>
struct ssl_composer
{
  StatePtr _state;
  enum
  {
    starting,
    sending,
    receiving,
  } _status = starting;

  template <typename Self>
  void operator()(Self& self, error_code ec = error_code{}, std::size_t n = 0)
  {
    if (!ec)
    {
      switch (_status)
      {
      case starting:
        _status = sending;
        http::async_write(_state->stream, _state->req, std::move(self));
        return;
      case sending:
        _status = receiving;
        http::async_read(
            _state->stream, _state->buffer, _state->res, std::move(self));
        return;
      case receiving:
        break;
      }
    }
    self.complete(ec);
  }
};
}
template <typename CompletionToken,
          typename AsyncStream,
          typename Request,
          typename Response>
auto async_process_one(AsyncStream& stream,
                       Request& request,
                       Response& response,
                       CompletionToken&& token) ->
    typename net::async_result<typename std::decay_t<CompletionToken>,
                               void(error_code)>::return_type

{
  auto state = std::make_unique<detail::state<AsyncStream, Request, Response>>(
      stream, request, response);
  return net::async_compose<CompletionToken, void(error_code)>(
      detail::ssl_composer<decltype(state)>{std::move(state)}, token, stream);
}
}
