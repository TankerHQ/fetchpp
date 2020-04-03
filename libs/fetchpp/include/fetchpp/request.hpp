#pragma once

#include <fetchpp/options.hpp>
#include <fetchpp/url.hpp>
#include <fetchpp/version.hpp>

#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/verb.hpp>
#include <fetchpp/alias/http.hpp>

namespace fetchpp
{
namespace detail
{
void set_options(options const& opt,
                 std::string const& domain,
                 http::fields& fields);
}

template <typename BodyType = http::empty_body>
class request : public http::request<BodyType>
{
  using base_t = http::request<BodyType>;

public:
  request(http::verb verb, url uri, options opt);

  fetchpp::options const& options() const;
  url const& uri() const;
  boost::string_view content_type() const;
  void content_type(beast::string_param const& param);

private:
  url _uri;
  fetchpp::options _opt;
};

template <typename BodyType = http::string_body,
          typename Value = typename BodyType::value_type>
request<BodyType> make_request(http::verb verb,
                               url uri,
                               options opt = {},
                               Value body = {});

inline auto make_request(http::verb verb, url uri, options opt = {})
{
  return request<http::empty_body>(verb, uri, opt);
}

// =================

template <typename BodyType>
request<BodyType>::request(http::verb verb, url uri, fetchpp::options opt)
  : base_t(verb, uri.target(), opt.version),
    _uri(std::move(uri)),
    _opt(std::move(opt))
{
  detail::set_options(_opt, _uri.domain(), *this);
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
  this->insert(beast::http::field::content_type, param);
}

template <typename BodyType = http::string_body,
          typename Value = typename BodyType::value_type>
request<BodyType> make_request(http::verb verb,
                               url uri,
                               options opt,
                               Value body)
{
  auto req = request<BodyType>(verb, uri, opt);
  req.body() = std::move(body);
  req.prepare_payload();
  return req;
}

}
