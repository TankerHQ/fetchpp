#pragma once

#include <fetchpp/core/process_one.hpp>
#include <fetchpp/core/session.hpp>
#include <fetchpp/core/ssl_transport.hpp>
#include <fetchpp/core/tcp_transport.hpp>

#include <fetchpp/core/detail/async_http_result.hpp>
#include <fetchpp/core/detail/coroutine.hpp>
#include <fetchpp/core/detail/endpoint.hpp>
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
template <typename Endpoint,
          typename AsyncTransport,
          typename Request,
          typename Response>
struct fetch_temporary_data
{
  Request req;
  session<Endpoint, AsyncTransport> sess;
  Response res;

  fetch_temporary_data(Endpoint&& endpoint,
                       Request&& preq,
                       AsyncTransport&& transport)
    : req(std::move(preq)),
      sess(std::move(endpoint), std::move(transport)),
      res()
  {
  }
};

template <typename Endpoint,
          typename AsyncTransport,
          typename Request,
          typename Response,
          typename CompletionHandler>
struct fetch_op : stable_async_t<AsyncTransport, CompletionHandler>
{
  using base_t = stable_async_t<AsyncTransport, CompletionHandler>;

  using temp_data_t =
      fetch_temporary_data<Endpoint, AsyncTransport, Request, Response>;
  temp_data_t& data;

  net::coroutine coro_;

  fetch_op(Endpoint&& endpoint,
           AsyncTransport&& transport,
           Request&& req,
           CompletionHandler&& handler)
    : base_t(std::move(handler), transport.get_executor()),
      data(beast::allocate_stable<temp_data_t>(
          *this, std::move(endpoint), std::move(req), std::move(transport)))
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

template <typename Endpoint,
          typename AsyncTransport,
          typename Response,
          typename Request,
          typename CompletionToken>
auto async_fetch_impl(Endpoint&& endpoint,
                      AsyncTransport&& transport,
                      Request request,
                      CompletionToken&& token)
    -> detail::async_http_return_type_t<CompletionToken, Response>
{
  using async_completion_t =
      detail::async_http_completion<CompletionToken, Response>;

  async_completion_t async_comp{token};
  request.keep_alive(false);
  detail::fetch_op<Endpoint,
                   AsyncTransport,
                   Request,
                   Response,
                   typename async_completion_t::completion_handler_type>(
      std::move(endpoint),
      std::move(transport),
      std::move(request),
      std::move(async_comp.completion_handler));
  return async_comp.result.get();
}

template <typename Request, typename Response>
struct fetch_composer_with_ssl
{
  Request request;
  net::executor ex;
  net::ssl::context sslc = net::ssl::context{net::ssl::context::tls_client};

  template <typename Self>
  void operator()(Self& self)
  {
    async_fetch_impl<secure_endpoint, ssl_async_transport, Response>(
        detail::to_endpoint<true>(request.uri()),
        ssl_async_transport(std::chrono::seconds(30), ex, sslc),
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
  auto endpoint = detail::to_endpoint<true>(request.uri());
  return async_fetch_impl<secure_endpoint, ssl_async_transport, Response>(
      std::move(endpoint),
      ssl_async_transport(std::chrono::seconds(30), ex, sslc),
      std::move(request),
      std::move(token));
}

template <typename Response, typename Request, typename CompletionToken>
auto async_fetch(net::executor ex, Request request, CompletionToken&& token)
    -> detail::async_http_return_type_t<CompletionToken, Response>
{
  if (http::is_ssl_involved(request.uri()))
    return net::async_compose<CompletionToken, void(error_code, Response)>(
        detail::fetch_composer_with_ssl<Request, Response>{std::move(request),
                                                           ex},
        token,
        ex);
  else
    return async_fetch_impl<plain_endpoint, tcp_async_transport, Response>(
        detail::to_endpoint<false>(request.uri()),
        tcp_async_transport(std::chrono::seconds(30), ex),
        std::move(request),
        std::forward<CompletionToken>(token));
}
}
