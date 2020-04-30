#include <boost/beast/http/message.hpp>

#include <fetchpp/alias/beast.hpp>

namespace fetchpp::http
{
template <typename Body>
using simple_response = beast::http::response<Body>;
}
