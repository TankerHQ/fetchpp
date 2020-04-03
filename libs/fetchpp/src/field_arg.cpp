#include <fetchpp/field_arg.hpp>

namespace fetchpp
{
field_arg::field_arg(fetchpp::field field, std::string value)
  : field(std::move(field)),
    field_name(to_string(field)),
    value(std::move(value))
{
}

field_arg::field_arg(std::string name, std::string value)
  : field(http::string_to_field(name)),
    field_name(std::move(name)),
    value(std::move(value))
{
}

}