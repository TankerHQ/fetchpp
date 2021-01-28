#include <fetchpp/core/basic_transport.hpp>

#include <boost/asio/ssl.hpp>

#include <boost/beast/http/error.hpp>
#include <fetchpp/alias/error_code.hpp>
#include <fetchpp/alias/net.hpp>
#include <fetchpp/alias/ssl.hpp>

namespace fetchpp
{
bool is_brutally_closed(error_code ec)
{
  return ec == net::error::eof || ec == net::error::connection_reset ||
         ec == net::ssl::error::stream_truncated ||
         ec == boost::system::errc::broken_pipe ||
         ec == beast::http::error::end_of_stream;
}
}
