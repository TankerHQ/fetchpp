#pragma once

// keep that here until
// https://github.com/boostorg/beast/issues/1913
// is fixed
#include <ostream>

#include <fetchpp/field.hpp>

#include <initializer_list>
#include <string>

namespace fetchpp
{
struct field_arg
{
  fetchpp::field field;
  std::string field_name;
  std::string value;

  field_arg() = delete;
  field_arg(field_arg const&) = default;
  field_arg(field_arg&&) = default;
  field_arg& operator=(field_arg&&) = default;
  field_arg& operator=(field_arg const&) = default;

  field_arg(fetchpp::field field, std::string value);
  field_arg(std::string field_name, std::string value);
};

using headers = std::initializer_list<field_arg>;
}
