#include <fetchpp/client.hpp>

namespace fetchpp
{
namespace detail
{
}
client::client(net::executor ex, std::chrono::nanoseconds timeout)
  : client(ex, timeout, net::ssl::context(net::ssl::context::tlsv12_client))
{
}

client::client(net::io_context& ioc, std::chrono::nanoseconds timeout)
  : client(ioc.get_executor(), timeout)
{
}

client::client(net::executor ex,
               std::chrono::nanoseconds timeout,
               net::ssl::context context)
  : strand_(ex), timeout_(timeout), context_(std::move(context))
{
  set_verify_peer(false);
}

client::client(net::io_context& ioc,
               std::chrono::nanoseconds timeout,
               net::ssl::context context)
  : client(ioc.get_executor(), timeout, std::move(context))
{
}

void client::set_verify_peer(bool v)
{
  this->context_.set_verify_mode(v ? net::ssl::verify_peer :
                                     net::ssl::verify_none);
}

auto client::get_executor() const -> executor_type
{
  return strand_;
}

std::size_t client::max_pending_per_session() const
{
  return max_pending_;
}

void client::set_max_pending_per_session(std::size_t pending)
{
  max_pending_ = pending;
}

std::size_t client::session_count() const
{
  return sessions_.size();
}

void client::add_proxy(http::proxy_match key, http::proxy newproxy)
{
  net::post(this->strand_, [k = std::move(key), p = std::move(newproxy), this] {
    this->proxies_.emplace(std::move(k), std::move(p));
  });
}

http::proxy_map const& client::proxies() const
{
  return this->proxies_;
}

net::ssl::context& client::context()
{
  return this->context_;
}

}
