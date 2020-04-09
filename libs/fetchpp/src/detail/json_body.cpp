#include <fetchpp/json_body.hpp>

namespace fetchpp::http::detail
{
// json_wrapper

json_wrapper::json_wrapper(json_wrapper::base_t&& j) : base_t(j)
{
}

json_wrapper& json_wrapper::operator=(base_t&& j)
{
  this->base_t::operator=(j);
  return *this;
}

json_wrapper::json_wrapper(nlohmann::json const& j) : nlohmann::json(j)
{
}

json_wrapper& json_wrapper::operator=(base_t const& j)
{
  this->base_t::operator=(j);
  return *this;
}

std::string const& json_wrapper::get_dump() const
{
  if (!dump_)
    dump_ = this->dump();
  return *dump_;
}

// json_reader

void json_reader::init(boost::optional<std::uint64_t> const& content_length,
                       error_code& ec)
{
  ec = {};
  if (content_length)
    payload_.reserve(*content_length);
}

void json_reader::finish(error_code& ec)
try
{
  body_ = json_wrapper::parse(payload_);
}
catch (json_wrapper::parse_error const&)
{
  // FIXME: find/create a better error code
  ec = http::error::bad_chunk;
}

// json_writer

void json_writer::init(error_code& ec)
{
  ec = {};
}

auto json_writer::get(error_code& ec)
    -> boost::optional<std::pair<const_buffers_type, bool>>
{
  ec = {};
  return {{net::buffer(body_.get_dump()), false}};
}

}
