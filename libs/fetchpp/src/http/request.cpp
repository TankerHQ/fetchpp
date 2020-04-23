#include <fetchpp/http/request.hpp>

namespace fetchpp
{
namespace detail
{
void set_options(options const& opt,
                 std::string const& host,
                 http::fields& fields)
{
  fields.set(http::field::host, host);
  fields.set(http::field::user_agent, USER_AGENT);
  fields.set(http::field::cache_control, to_string(opt.cache));
  // FIXME: redirect handling
}
}
}
