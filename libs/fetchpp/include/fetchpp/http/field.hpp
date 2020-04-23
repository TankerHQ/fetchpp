#pragma once

#include <ostream> // https://github.com/boostorg/beast/issues/1913

#include <boost/beast/http/field.hpp>

#include <fetchpp/alias/http.hpp>

namespace fetchpp::http
{
using beast::http::field;
}
