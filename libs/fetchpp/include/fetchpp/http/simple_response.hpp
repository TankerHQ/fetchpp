#include <boost/beast/http/message.hpp>

#include <fetchpp/alias/beast.hpp>

namespace fetchpp
{
template <typename Body>
using simple_response = beast::http::response<Body>;
}
