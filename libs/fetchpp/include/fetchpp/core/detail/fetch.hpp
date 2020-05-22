#pragma once

#include <fetchpp/core/connect.hpp>
#include <fetchpp/core/process_one.hpp>
#include <fetchpp/core/session.hpp>
#include <fetchpp/core/ssl_transport.hpp>
#include <fetchpp/core/tcp_transport.hpp>

#include <fetchpp/core/detail/async_http_result.hpp>
#include <fetchpp/core/detail/coroutine.hpp>
#include <fetchpp/core/detail/http_stable_async.hpp>

#include <boost/beast/core/bind_handler.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>

#include <fetchpp/alias/error_code.hpp>
#include <fetchpp/alias/net.hpp>
#include <fetchpp/alias/ssl.hpp>

namespace fetchpp::detail
{
template <typename AsyncTransport, typename Request, typename Response>
struct fetch_temporary_data
{
  Request req;
  session<AsyncTransport> sess;
  Response res;

  fetch_temporary_data(Request&& preq, AsyncTransport&& transport)
    : req(std::move(preq)), sess(std::move(transport)), res()
  {
  }
};

template <typename AsyncTransport,
          typename Request,
          typename Response,
          typename CompletionHandler>
struct fetch_op : stable_async_t<AsyncTransport, CompletionHandler>
{
  using base_t = stable_async_t<AsyncTransport, CompletionHandler>;

  using temp_data_t = fetch_temporary_data<AsyncTransport, Request, Response>;
  temp_data_t& data;

  net::coroutine coro_;

  fetch_op(AsyncTransport&& transport,
           Request&& req,
           CompletionHandler&& handler)
    : base_t(std::move(handler), transport.get_executor()),
      data(beast::allocate_stable<temp_data_t>(
          *this, std::move(req), std::move(transport)))
  {
    (*this)();
  }

  void operator()(error_code ec = {})
  {
    FETCHPP_REENTER(coro_)
    {
      FETCHPP_YIELD data.sess.push_request(
          data.req, data.res, std::move(*this));
      // pin the response before temp_data_t is destroyed
      auto res = std::move(data.res);
      this->complete_now(ec, std::move(res));
    }
  }
};

template <typename AsyncTransport,
          typename Response,
          typename Request,
          typename CompletionToken>
auto async_fetch_impl(AsyncTransport&& transport,
                      Request request,
                      CompletionToken&& token)
    -> detail::async_http_return_type_t<CompletionToken, Response>
{
  using async_completion_t =
      detail::async_http_completion<CompletionToken, Response>;

  async_completion_t async_comp{token};
  request.keep_alive(false);
  detail::fetch_op<AsyncTransport,
                   Request,
                   Response,
                   typename async_completion_t::completion_handler_type>(
      std::move(transport),
      std::move(request),
      std::move(async_comp.completion_handler));
  return async_comp.result.get();
}

template <typename Request, typename Response>
struct fetch_composer_with_ssl
{
  std::string domain;
  uint16_t port;
  Request request;
  net::executor ex;
  net::ssl::context sslc = net::ssl::context{net::ssl::context::tls_client};

  template <typename Self>
  void operator()(Self& self)
  {
    // sslc.set_verify_mode(context::verify_peer);
    auto const uri = request.uri();
    async_fetch_impl<ssl_async_transport, Response>(
        ssl_async_transport(uri.domain(), uri.port(), ex, sslc),
        std::move(request),
        std::move(self));
  }

  template <typename Self>
  void operator()(Self& self, error_code ec, Response&& response)
  {
    self.complete(ec, std::move(response));
  }
};

template <typename Response, typename Request, typename CompletionToken>
auto async_fetch(net::executor ex,
                 net::ssl::context& sslc,
                 Request request,
                 CompletionToken&& token)
    -> detail::async_http_return_type_t<CompletionToken, Response>
{
  auto domain = request.uri().domain();
  auto port = request.uri().port();
  return async_fetch_impl<ssl_async_transport, Response>(
      ssl_async_transport(std::move(domain), port, ex, sslc),
      std::move(request),
      std::move(token));
}

template <typename Response, typename Request, typename CompletionToken>
auto async_fetch(net::executor ex, Request request, CompletionToken&& token)
    -> detail::async_http_return_type_t<CompletionToken, Response>
{
  auto is_ssl = request.uri().is_ssl_involved();
  auto domain = request.uri().domain();
  auto port = request.uri().port();
  if (is_ssl)
  {
    return net::async_compose<CompletionToken, void(error_code, Response)>(
        detail::fetch_composer_with_ssl<Request, Response>{
            std::move(domain), port, std::move(request), ex},
        token,
        ex);
  }
  else
  {
    return async_fetch_impl<tcp_async_transport, Response>(
        tcp_async_transport(std::move(domain), port, ex),
        std::move(request),
        std::move(token));
  }
}

}
