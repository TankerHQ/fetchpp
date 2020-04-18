#pragma once

#include <fetchpp/connect.hpp>
#include <fetchpp/process_one.hpp>

#include <fetchpp/detail/async_http_result.hpp>
#include <fetchpp/detail/http_stable_async.hpp>

#include <fetchpp/alias/error_code.hpp>
#include <fetchpp/alias/net.hpp>

namespace fetchpp::detail
{
template <typename AsyncStream, typename Request, typename Response>
struct fetch_temporary_data
{
  AsyncStream stream;
  Request req;
  Response res;
  beast::flat_buffer buffer;
  tcp::resolver resolver;
  fetch_temporary_data(AsyncStream&& pstream, Request&& req)
    : stream(std::move(pstream)),
      req(std::move(req)),
      res(),
      buffer(),
      resolver(stream.get_executor())
  {
  }
};

template <typename AsyncStream,
          typename Response,
          typename Request,
          typename CompletionToken>
struct fetch_composer : stable_async_t<AsyncStream, Response, CompletionToken>
{
  using base_type_t = stable_async_t<AsyncStream, Response, CompletionToken>;
  using handler_type =
      typename detail::async_http_completion_handler_t<CompletionToken,
                                                       Response>;
  using temp_data_t = fetch_temporary_data<AsyncStream, Request, Response>;
  enum status : int
  {
    starting,
    resolving,
    connecting,
    processing,
  };

  temp_data_t& data;
  status state = status::starting;

  fetch_composer(AsyncStream&& stream, Request&& preq, handler_type& handler)
    : base_type_t(std::move(handler), stream.get_executor()),
      data(beast::allocate_stable<temp_data_t>(
          *this, std::move(stream), std::move(preq)))
  {
    (*this)();
  }

  void operator()(error_code ec = {})
  {
    if (!ec)
    {
      switch (state)
      {
      case starting:
        state = status::resolving;
        data.resolver.async_resolve(data.req.uri().domain(),
                                    std::to_string(data.req.uri().port()),
                                    net::ip::resolver_base::numeric_service,
                                    std::move(*this));
        return;
      case connecting:
        state = status::processing;
        async_process_one(data.stream, data.req, data.res, std::move(*this));
        return;
      default:
        assert(0);
        break;
      case processing:
        break;
      }
    }
    this->complete_now(ec, std::move(std::exchange(data.res, Response{})));
  }

  void operator()(error_code ec, tcp::resolver::results_type results)
  {
    if (ec)
    {
      this->complete_now(ec, std::move(std::exchange(data.res, Response{})));
      return;
    }
    state = status::connecting;
    async_connect(
        data.stream, data.req.uri().domain(), results, std::move(*this));
  }
};

template <typename Response, typename Request, typename CompletionToken>
auto async_fetch_impl(net::io_context& ioc,
                      net::ssl::context& sslc,
                      Request request,
                      CompletionToken&& token)
    -> detail::async_http_return_type_t<CompletionToken, Response>
{
  using async_completion_t =
      detail::async_http_completion<CompletionToken, Response>;
  using AsyncStream = beast::ssl_stream<beast::tcp_stream>;
  AsyncStream stream(ioc, sslc);

  async_completion_t async_comp{token};
  request.keep_alive(false);
  detail::fetch_composer<AsyncStream, Response, Request, CompletionToken>(
      std::move(stream), std::move(request), async_comp.completion_handler);
  return async_comp.result.get();
}

template <typename Request, typename Response>
struct fetch_composer_with_ssl
{
  net::io_context& ioc;
  Request request;
  net::ssl::context sslc = net::ssl::context{net::ssl::context::tls_client};

  template <typename Self>
  void operator()(Self& self)
  {
    async_fetch_impl<Response>(ioc, sslc, std::move(request), std::move(self));
    return;
  }

  template <typename Self>
  void operator()(Self& self, error_code ec, Response&& response)
  {
    self.complete(ec, std::move(response));
  }
};

}
