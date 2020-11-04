#pragma once

#include <fetchpp/http/content_type.hpp>

#include <fetchpp/http/message.hpp>

#include <boost/beast/core/multi_buffer.hpp>

#include <fetchpp/alias/http.hpp>
#include <fetchpp/alias/net.hpp>

#include <nlohmann/json.hpp>

#include <optional>

namespace fetchpp::http
{

template <typename DynamicBuffer>
class basic_response : public message<false, DynamicBuffer>
{
public:
  using base_t = message<false, DynamicBuffer>;
  using base_t::operator=;
  using base_t::base_t;

  bool ok() const
  {
    return to_status_class(this->result()) ==
           beast::http::status_class::successful;
  }
};

using response = basic_response<beast::multi_buffer>;
}
