#include <fetchpp/http/detail/request.hpp>

#include <fetchpp/version.hpp>

namespace fetchpp::http::detail
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
