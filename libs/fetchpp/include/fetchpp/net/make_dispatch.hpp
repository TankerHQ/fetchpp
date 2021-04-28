#pragma once

#include <boost/asio/dispatch.hpp>

#include <fetchpp/alias/net.hpp>

namespace fetchpp::net
{
template <typename Handler, typename Executor>
auto make_dispatch(Handler handler, Executor default_executor)
{
  auto ex = net::get_associated_executor(handler, default_executor);
  return net::dispatch(ex, std::move(handler));
}
}
