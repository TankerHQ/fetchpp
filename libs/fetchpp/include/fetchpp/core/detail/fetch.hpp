#pragma once

#include <fetchpp/core/detail/coroutine.hpp>
#include <fetchpp/core/detail/endpoint.hpp>
#include <fetchpp/core/process_one.hpp>
#include <fetchpp/core/tcp_transport.hpp>

#include <fetchpp/net/make_post.hpp>

#include <boost/asio/dispatch.hpp>
#include <boost/asio/executor.hpp>

#include <boost/beast/core/bind_handler.hpp>

#include <fetchpp/alias/error_code.hpp>
#include <fetchpp/alias/net.hpp>

namespace fetchpp::detail
{
template <typename Endpoint,
          typename AsyncTransport,
          typename Response,
          typename Request,
          typename Handler>
struct base_state
{
  using transport_type = AsyncTransport;
  Endpoint endpoint;
  AsyncTransport transport;
  Request request;
  Handler handler;
  Response response = {};
};

template <template <typename...> class State,
          typename Response,
          typename AsyncTransport,
          typename Endpoint,
          typename Request,
          typename Handler>
auto make_state(Endpoint&& ep, AsyncTransport&& tr, Request&& rq, Handler&& h)
{
  using state = State<std::decay_t<Endpoint>,
                      std::decay_t<AsyncTransport>,
                      Response,
                      std::decay_t<Request>,
                      std::decay_t<Handler>>;
  return std::unique_ptr<state>(new state{std::forward<Endpoint>(ep),
                                          std::forward<AsyncTransport>(tr),
                                          std::forward<Request>(rq),
                                          std::forward<Handler>(h)});
}
template <typename State>
struct simple_fetch_op
{
  std::unique_ptr<State> state_;
  net::coroutine coro_{};
  auto& state()
  {
    return *state_;
  }
  auto& transport()
  {
    return state().transport;
  }
  auto& endpoint()
  {
    return state().endpoint;
  }

  void operator()(error_code ec = {})
  {
    FETCHPP_REENTER(coro_)
    {
      FETCHPP_YIELD transport().async_connect(endpoint(), std::move(*this));
      FETCHPP_YIELD async_process_one(
          transport(), state().request, state().response, std::move(*this));
      net::make_post(
          beast::bind_front_handler(
              std::move(state().handler), ec, std::move(state().response)),
          get_executor());
    }
  }
  using executor_type = typename State::transport_type::executor_type;
  auto get_executor() const
  {
    return state_->transport.get_executor();
  }
};
template <typename State>
simple_fetch_op(std::unique_ptr<State>) -> simple_fetch_op<State>;

template <typename Response, typename Request, typename CompletionToken>
auto async_fetch(net::any_io_executor ex, Request&& request, CompletionToken&& token)
{
  auto launch = [](auto&& handler, net::any_io_executor ex, Request request) {
    auto s = make_state<base_state, Response>(
        detail::to_endpoint<false>(request.uri()),
        tcp_async_transport(ex, std::chrono::seconds(30)),
        std::forward<Request>(request),
        std::forward<decltype(handler)>(handler));
    net::dispatch(simple_fetch_op{std::move(s)});
  };
  return net::async_initiate<CompletionToken, void(error_code, Response)>(
      std::move(launch), token, ex, std::forward<Request>(request));
}
}
