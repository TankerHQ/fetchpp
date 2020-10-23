#include <fetchpp/version.hpp>

#include <boost/beast/http/fields.hpp>

#include <fetchpp/alias/http.hpp>

#include <string>

namespace fetchpp::http::detail
{
void set_options(std::string const& host, http::fields& fields)
{
  fields.set(http::field::host, host);
  fields.set(http::field::user_agent, USER_AGENT);
  fields.set(http::field::cache_control, "no-cache");
  // FIXME: redirect handling
}
}
