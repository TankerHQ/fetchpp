#pragma once

#include <boost/beast/core/stream_traits.hpp>

#include <fetchpp/alias/beast.hpp>
#include <fetchpp/alias/net.hpp>

namespace fetchpp
{
template <class T>
using is_async_transport = std::integral_constant<
    bool,
    beast::is_async_stream<T>::value &&
        net::is_dynamic_buffer<typename T::buffer_type>::value>;

}
