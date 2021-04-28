#pragma once

#include <boost/asio/post.hpp>

#include <fetchpp/alias/net.hpp>

namespace fetchpp::net
{
template <typename Handler, typename Executor>
void make_post(Handler handler, Executor default_executor)
{
  auto ex = net::get_associated_executor(handler, default_executor);
  return net::post(ex, std::move(handler));
}
}
