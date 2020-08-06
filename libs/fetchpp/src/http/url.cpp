#include <fetchpp/http/url.hpp>

#include <nlohmann/json.hpp>
#include <skyr/json/json.hpp>

#include <exception>
#include <string_view>

namespace fetchpp::http
{

url::string_type url::target() const
{
  return this->pathname() + this->search() + this->hash();
}

std::int16_t safe_port(url const& uri)
{
  if (uri.port().empty())
  {
    auto const proto = uri.protocol();
    auto const scheme = std::string_view{proto.data(), proto.size() - 1};
    if (auto port = url::default_port(scheme); port.has_value())
      return *port;
  }
  else if (auto port = uri.port<std::int16_t>(); port.has_value())
  {
    return *port;
  }
  throw std::domain_error("cannot deduce port");
}

bool is_ssl_involved(url const& uri)
{
  if (!uri.protocol().empty())
    return uri.protocol() == "https:";
  return uri.port<std::int16_t>() == 443;
}

nlohmann::json decode_query(std::string_view query)
{
  return skyr::json::decode_query(query);
}

std::string encode_query(nlohmann::json const& json)
{
  return skyr::json::encode_query(json).value();
}

}
