#pragma once

#include <string>

#include <nlohmann/json.hpp>

#include <fetchpp/alias/error_code.hpp>
#include <fetchpp/alias/http.hpp>

#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/http/message.hpp>

#include <optional>

namespace fetchpp
{
/// We need that to maintain the dump() string,
/// which has to outlive the json_body.
class json_wrapper : public nlohmann::json
{
public:
  using base_t = nlohmann::json;
  json_wrapper(base_t&& j) : base_t(j)
  {
  }

  json_wrapper& operator=(base_t&& j)
  {
    this->base_t::operator=(j);
    return *this;
  }

  json_wrapper(nlohmann::json const& j) : nlohmann::json(j)
  {
  }

  json_wrapper& operator=(base_t const& j)
  {
    this->base_t::operator=(j);
    return *this;
  }

  json_wrapper(json_wrapper&&) = default;
  json_wrapper& operator=(json_wrapper&&) = default;
  json_wrapper() = default;

  json_wrapper& operator=(json_wrapper const&) = delete;
  json_wrapper(json_wrapper const&) = delete;

  inline std::string const& get_dump() const
  {
    if (!dump_)
      dump_ = this->dump();
    return *dump_;
  }

private:
  mutable std::optional<std::string> dump_;
};

struct json_body
{
  using value_type = json_wrapper;
  using const_buffers_type = net::const_buffer;

  class reader
  {
  public:
    template <bool isRequest, typename Fields>
    explicit reader(http::header<isRequest, Fields>&, value_type& b) : body_(b)
    {
    }

    void init(boost::optional<std::uint64_t> const& content_length,
              error_code& ec)
    {
      ec = {};
      if (content_length)
        payload_.reserve(*content_length);
    }

    template <typename ConstBufferSequence>
    std::size_t put(ConstBufferSequence const& buffers, error_code& ec)
    {
      // yakety sax playing in the background...
      auto const before = payload_.size();
      payload_.append(beast::buffers_to_string(buffers));
      ec = {};
      return payload_.size() - before;
    }

    void finish(error_code& ec)
    try
    {
      body_ = value_type::parse(payload_);
    }
    catch (value_type::parse_error const&)
    {
      // FIXME: find/create a better error code
      ec = http::error::bad_chunk;
    }

  private:
    std::string payload_;
    value_type& body_;
  };

  class writer
  {
  public:
    template <bool isRequest, typename Fields>
    writer(http::header<isRequest, Fields>& h, value_type& b) : body_(b)
    {
      if (h.find(http::field::content_type) == h.end())
        h.set(http::field::content_type, "application/json");
    }

    using const_buffers_type = net::const_buffer;

    void init(error_code& ec)
    {
      ec = {};
    }

    auto get(error_code& ec)
        -> boost::optional<std::pair<const_buffers_type, bool>>
    {
      ec = {};
      return {{net::buffer(body_.get_dump()), false}};
    }

  private:
    value_type& body_;
  };
};
}
