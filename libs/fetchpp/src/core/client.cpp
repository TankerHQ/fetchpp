#include <fetchpp/client.hpp>

namespace fetchpp
{
namespace detail
{
}
client::client(net::executor ex)
  : strand_(ex), context_(net::ssl::context::tlsv12_client)
{
}

client::client(net::io_context& ioc) : client(ioc.get_executor())
{
}

client::client(net::executor ex, net::ssl::context context)
  : strand_(ex), context_(std::move(context))
{
}

client::client(net::io_context& ioc, net::ssl::context context)
  : client(ioc.get_executor(), std::move(context))
{
}

auto client::get_executor() const -> executor_type
{
  return strand_;
}
}
