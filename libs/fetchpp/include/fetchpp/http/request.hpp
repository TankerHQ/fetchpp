#pragma once

#include <fetchpp/core/options.hpp>
#include <fetchpp/http/authorization.hpp>
#include <fetchpp/http/url.hpp>
#include <fetchpp/version.hpp>

#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/verb.hpp>

#include <fetchpp/alias/http.hpp>

#include <optional>

namespace fetchpp
{
namespace detail
{
void set_options(options const& opt,
                 std::string const& host,
                 http::fields& fields);
}

template <typename BodyType = http::empty_body>
class request : public beast::http::request<BodyType>
{
  using base_t = beast::http::request<BodyType>;

public:
  request(http::verb verb, url uri, options opt);

  fetchpp::options const& options() const;
  url const& uri() const;
  std::string_view content_type() const;
  void content_type(beast::string_param const& param);

  using base_t::set;
  void set(http::authorization::methods const& m);

private:
  url _uri;
  fetchpp::options _opt;
};

template <typename Body>
std::optional<std::string_view> select_content_type(Body const&)
{
  return std::nullopt;
}

template <typename Request = request<http::string_body>,
          typename Value = typename Request::body_type::value_type>
Request make_request(http::verb verb,
                     url uri,
                     options opt = {},
                     Value body = {});

inline auto make_request(http::verb verb, url uri, options opt = {})
{
  return request<http::empty_body>(verb, uri, std::move(opt));
}

// =================

template <typename BodyType>
request<BodyType>::request(http::verb verb, url uri, fetchpp::options opt)
  : base_t(verb,
           uri.target(),
           static_cast<std::underlying_type_t<http_version>>(opt.version)),
    _uri(std::move(uri)),
    _opt(std::move(opt))
{
  detail::set_options(_opt, _uri.host(), *this);
}

template <typename BodyType>
options const& request<BodyType>::options() const
{
  return _opt;
}

template <typename BodyType>
url const& request<BodyType>::uri() const
{
  return _uri;
}

template <typename BodyType>
void request<BodyType>::content_type(beast::string_param const& param)
{
  this->insert(http::field::content_type, param);
}

template <typename BodyType>
void request<BodyType>::set(http::authorization::methods const& m)
{
  this->set(fetchpp::http::field::authorization, m);
}

template <typename Request, typename Value>
Request make_request(http::verb verb, url uri, options opt, Value body)
{
  auto req = Request(verb, uri, opt);
  if (auto ct = select_content_type(req.body()); ct.has_value())
    req.content_type(*ct);
  req.body() = std::move(body);
  req.prepare_payload();
  return req;
}

}
