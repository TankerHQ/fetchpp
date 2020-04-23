#pragma once

#include <fetchpp/core/field_arg.hpp>

#include <initializer_list>

namespace fetchpp::http
{
using headers = std::initializer_list<field_arg>;
}
