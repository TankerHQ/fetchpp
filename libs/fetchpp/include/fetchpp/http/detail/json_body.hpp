#include <nlohmann/json.hpp>

#include <boost/asio/buffer.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/http/message.hpp>

#include <fetchpp/alias/error_code.hpp>
#include <fetchpp/alias/http.hpp>
#include <fetchpp/alias/net.hpp>

#include <optional>
#include <string>

namespace fetchpp::http::detail
{
/// We need that to maintain the dump() string,
/// which has to outlive the json_body.
class json_wrapper : public nlohmann::json
{
public:
  using base_t = nlohmann::json;
  json_wrapper(base_t&& j);

  json_wrapper& operator=(base_t&& j);

  json_wrapper(nlohmann::json const& j);
  json_wrapper& operator=(base_t const& j);

  json_wrapper(json_wrapper&&) = default;
  json_wrapper& operator=(json_wrapper&&) = default;
  json_wrapper() = default;

  json_wrapper& operator=(json_wrapper const&) = delete;
  json_wrapper(json_wrapper const&) = delete;

  std::string const& get_dump() const;

private:
  mutable std::optional<std::string> dump_;
};

inline constexpr std::optional<std::string_view> select_content_type(
    json_wrapper const&)
{
  return "application/json";
}

class json_reader
{
public:
  template <bool isRequest, typename Fields>
  explicit json_reader(http::header<isRequest, Fields>&, json_wrapper& b)
    : body_(b)
  {
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

  void init(boost::optional<std::uint64_t> const& content_length,
            error_code& ec);
  void finish(error_code& ec);

private:
  std::string payload_;
  json_wrapper& body_;
};

class json_writer
{
public:
  using const_buffers_type = net::const_buffer;

  template <bool isRequest, typename Fields>
  json_writer(http::header<isRequest, Fields> const&, json_wrapper const& b)
    : body_(b)
  {
  }

  void init(error_code& ec);

  auto get(error_code& ec)
      -> boost::optional<std::pair<const_buffers_type, bool>>;

private:
  json_wrapper const& body_;
};
}
