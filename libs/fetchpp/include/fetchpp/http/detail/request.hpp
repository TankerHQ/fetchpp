#pragma once

#include <fetchpp/core/options.hpp>
#include <fetchpp/http/field.hpp>

#include <boost/beast/http/fields.hpp>

#include <fetchpp/alias/http.hpp>

namespace fetchpp::http::detail
{
void set_options(options const& opt,
                 std::string const& host,
                 http::fields& fields);
}
