#pragma once

#include <boost/beast/http/message.hpp>

#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/span_body.hpp>
#include <boost/beast/http/string_body.hpp>

#include <fetchpp/alias/http.hpp>

namespace fetchpp
{
// using http::request;
using http::response;

using http::empty_body;
using http::span_body;
using http::string_body;
}
