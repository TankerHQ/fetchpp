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
template <typename AsyncStream,
          typename Buffer,
          typename Request,
          typename Body,
          typename CompletionToken>
auto async_process_one(AsyncStream& stream,
                       Buffer& buffer,
                       Request& request,
                       beast::http::response_parser<Body>& parser,
                       CompletionToken&& token) ->
    typename net::async_result<std::decay_t<CompletionToken>,
                               void(error_code)>::return_type;

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
                               void(error_code)>::return_type;

template <typename AsyncTransport,
          typename Request,
          typename Response,
          typename CompletionToken>
auto async_process_one(AsyncTransport& transport,
                       Request& request,
                       Response& response,
                       CompletionToken&& token) ->
    typename net::async_result<std::decay_t<CompletionToken>,
                               void(error_code)>::return_type;

namespace process_one::detail
{
template <typename AsyncStream,
          typename Buffer,
          typename ResponseParser,
          typename CompletionToken,
          std::enable_if_t<ResponseParser::is_request::value == false, int> = 0>
auto run_async_read(AsyncStream& stream,
                    Buffer& buffer,
                    ResponseParser& parser,
                    CompletionToken&& token)
{
  return http::async_read(stream, buffer, parser, std::move(token));
}

template <typename AsyncStream,
          typename Request,
          typename CompletionToken,
          std::enable_if_t<Request::is_request::value == true, int> = 0>
auto run_async_write(AsyncStream& stream,
                     Request& request,
                     CompletionToken&& token)
{
  return http::async_write(stream, request, std::move(token));
}

template <typename AsyncStream,
          typename Request,
          typename ResponseParser,
          typename Buffer>
struct parser_op
{
  AsyncStream& stream;
  Request& req;
  ResponseParser& parser;
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
      FETCHPP_YIELD run_async_read(stream, buffer, parser, std::move(self));
      self.complete(ec);
    }
  }
};
template <typename AsyncStream,
          typename Request,
          typename ResponseParser,
          typename Buffer>
parser_op(AsyncStream&, Request&, ResponseParser&, Buffer&)
    -> parser_op<AsyncStream, Request, ResponseParser, Buffer>;

template <typename AsyncStream,
          typename Request,
          typename Response,
          typename Buffer>
struct stream_op
{
  AsyncStream& stream_;
  Request& req_;
  Response& res_;
  Buffer& buffer_;
  using parser_t = beast::http::response_parser<typename Response::body_type>;
  std::unique_ptr<parser_t> parser_;
  net::coroutine coro_ = net::coroutine{};

  stream_op(AsyncStream& stream, Request& req, Response& res, Buffer& buf)
    : stream_(stream),
      req_(req),
      res_(res),
      buffer_(buf),
      parser_(std::make_unique<parser_t>())
  {
    parser_->body_limit(83'886'080); // 80MiB
  }

  template <typename Self>
  void operator()(Self& self, error_code ec = error_code{}, std::size_t = 0)
  {
    if (ec)
    {
      self.complete(ec);
      return;
    }

    FETCHPP_REENTER(coro_)
    {
      if (req_.method() == beast::http::verb::connect ||
          req_.method() == beast::http::verb::head)
        parser_->skip(true);

      FETCHPP_YIELD async_process_one(
          stream_, buffer_, req_, *parser_, std::move(self));
      res_ = std::move(parser_->release());
      self.complete(ec);
    }
  }
};

template <typename AsyncTransport, typename Request, typename Response>
struct transport_op
{
  AsyncTransport& transport_;
  Request& req_;
  Response& res_;
  using parser_t = beast::http::response_parser<typename Response::body_type>;
  std::unique_ptr<parser_t> parser_;
  net::coroutine coro_ = net::coroutine{};

  transport_op(AsyncTransport& transport, Request& req, Response& res)
    : transport_(transport),
      req_(req),
      res_(res),
      parser_(std::make_unique<parser_t>())
  {
    parser_->body_limit(80 * 1024 * 1024);
    if (req_.method() == beast::http::verb::connect ||
        req_.method() == beast::http::verb::head)
      parser_->skip(true);
  }

  template <typename Self>
  void operator()(Self& self, error_code ec = error_code{}, std::size_t = 0)
  {
    if (ec)
    {
      transport_.cancel_timer();
      self.complete(ec);
      return;
    }
    if (!transport_.is_running())
    {
      transport_.cancel_timer();
      self.complete(net::error::operation_aborted);
      return;
    }
    FETCHPP_REENTER(coro_)
    {
      transport_.setup_timer();
      FETCHPP_YIELD detail::run_async_write(
          transport_.next_layer(), req_, std::move(self));
      FETCHPP_YIELD detail::run_async_read(transport_.next_layer(),
                                           transport_.buffer(),
                                           *parser_,
                                           std::move(self));
      res_ = std::move(parser_->release());
      transport_.cancel_timer();
      self.complete(ec);
    }
  }
};
}

// ======

template <typename AsyncStream,
          typename Buffer,
          typename Request,
          typename Body,
          typename CompletionToken>
auto async_process_one(AsyncStream& stream,
                       Buffer& buffer,
                       Request& request,
                       beast::http::response_parser<Body>& parser,
                       CompletionToken&& token) ->
    typename net::async_result<std::decay_t<CompletionToken>,
                               void(error_code)>::return_type
{
  return net::async_compose<CompletionToken, void(error_code)>(
      process_one::detail::parser_op{stream, request, parser, buffer},
      token,
      stream);
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
      process_one::detail::stream_op(stream, request, response, buffer),
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
      process_one::detail::transport_op<AsyncTransport, Request, Response>{
          transport, request, response},
      token,
      transport);
}
}
