#include <fetchpp/request.hpp>

namespace fetchpp
{
namespace detail
{
void set_options(options const& opt,
                 std::string const& domain,
                 http::fields& fields)
{
  fields.set(http::field::host, domain);
  fields.set(http::field::user_agent, USER_AGENT);
  fields.set(http::field::cache_control, to_string(opt.cache));
  // FIXME: redirect handling
}
}
}
