#pragma once

#include <boost/asio/ssl/context.hpp>

/*  Load a signed certificate into the ssl context, and configure
    the context for use with a server.
*/
namespace test::helpers
{
void load_server_certificate(boost::asio::ssl::context& ctx);
}
