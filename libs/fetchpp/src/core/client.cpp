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

net::ssl::context& client::context()
{
  return this->context_;
}

auto client::get_executor() const -> executor_type
{
  return strand_;
}
}
