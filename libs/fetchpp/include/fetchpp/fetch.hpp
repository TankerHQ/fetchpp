#pragma once

#include <fetchpp/connect.hpp>
#include <fetchpp/process_one.hpp>

#include <fetchpp/detail/async_http_result.hpp>

#include <boost/beast/core/tcp_stream.hpp>

#include <fetchpp/alias/beast.hpp>
#include <fetchpp/alias/error_code.hpp>
#include <fetchpp/alias/http.hpp>
#include <fetchpp/alias/net.hpp>

namespace fetchpp
{
template <typename Response, typename Request, typename GetHandler>
auto async_fetch(net::io_context& ioc, Request request, GetHandler&& handler)
    -> detail::async_http_result_t<GetHandler, Response>
{
  using async_completion_t =
      net::async_completion<GetHandler, void(error_code, Response)>;
  using handler_type = typename async_completion_t::completion_handler_type;
  using AsyncStream = beast::ssl_stream<beast::tcp_stream>;

  net::ssl::context sslc(net::ssl::context::tlsv12_client);
  AsyncStream stream(ioc, sslc);

  using base_type =
      beast::stable_async_base<handler_type,
                               typename AsyncStream::executor_type>;

  struct op : base_type
  {
    enum status : int
    {
      starting,
      resolving,
      connecting,
      processing,
    };

    struct temporary_data
    {
      AsyncStream stream;
      Request req;
      Response res;
      beast::flat_buffer buffer;
      tcp::resolver resolver;
      temporary_data(AsyncStream pstream, Request req)
        : stream(std::move(pstream)),
          req(std::move(req)),
          res(),
          buffer(),
          resolver(stream.get_executor())
      {
      }
    };

    temporary_data& data;
    status state = status::starting;

    op(AsyncStream stream, Request preq, handler_type& handler)
      : base_type(std::move(handler), stream.get_executor()),
        data(beast::allocate_stable<temporary_data>(
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
                                      data.req.uri().scheme(),
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
      async_connect(data.stream, results, std::move(*this));
    }
  };

  async_completion_t async_comp{handler};
  op(std::move(stream), std::move(request), async_comp.completion_handler);
  return async_comp.result.get();
}

}
