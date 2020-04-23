#pragma once

#include <boost/asio/coroutine.hpp>

#define FETCHPP_REENTER(c) BOOST_ASIO_CORO_REENTER(c)

#define FETCHPP_YIELD BOOST_ASIO_CORO_YIELD

namespace fetchpp::net
{
using boost::asio::coroutine;
}
