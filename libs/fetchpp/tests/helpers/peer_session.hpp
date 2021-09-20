
#include <boost/asio/basic_socket_acceptor.hpp>
#include <boost/asio/compose.hpp>
#include <boost/asio/dispatch.hpp>

#include <boost/beast/core/detail/get_io_context.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/write.hpp>

#include <nlohmann/json.hpp>

#include <future>

#include <fmt/format.h>
#include <fmt/ostream.h>

namespace test::helpers
{
namespace bb = boost::beast;
namespace net = boost::asio;

using string_view = bb::string_view;

namespace detail
{
template <typename PeerSession>
struct reply_back_op
{
  PeerSession& session_;
  bb::http::status status_;
  using request_type = typename PeerSession::request_type;
  using response_type = typename PeerSession::response_type;
  std::unique_ptr<request_type> request_ = nullptr;
  net::coroutine coro_ = {};

  template <typename Self>
  void operator()(Self& self, bb::error_code ec = {})
  {
    if (ec)
    {
      self.complete(ec);
      return;
    }
    BOOST_ASIO_CORO_REENTER(coro_)
    {
      request_ = std::make_unique<request_type>();
      BOOST_ASIO_CORO_YIELD session_.async_receive(*request_, std::move(self));
      BOOST_ASIO_CORO_YIELD session_.async_send(
          status_,
          "application/json",
          nlohmann::json({{"data", request_->body()}}).dump(),
          std::move(self));
      self.complete(ec);
    }
  }
};
template <typename PeerSession>
reply_back_op(PeerSession&, bb::http::status) -> reply_back_op<PeerSession>;

template <typename PeerSession>
struct receive_some_op
{
  PeerSession& session_;
  std::size_t at_most_;
  net::coroutine coro_ = {};

  template <typename Self>
  void operator()(Self& self, bb::error_code ec = {}, std::size_t read = 0)
  {
    if (ec)
    {
      self.complete(ec);
      return;
    }
    BOOST_ASIO_CORO_REENTER(coro_)
    {
      BOOST_ASIO_CORO_YIELD session_.stream_->async_read_some(
          session_.buffer_.prepare(at_most_), std::move(self));
      session_.buffer_.commit(read);
      self.complete(ec);
    }
  }
};
template <typename PeerSession>
receive_some_op(PeerSession&, std::size_t) -> receive_some_op<PeerSession>;

template <typename PeerSession>
struct send_some_op
{
  using response_type = typename PeerSession::response_type;
  using parser_type =
      bb::http::response_serializer<typename response_type::body_type>;
  PeerSession& session_;
  response_type response_;
  std::unique_ptr<parser_type> sr_;
  std::size_t at_most_;
  net::coroutine coro_;

  template <typename Response>
  send_some_op(PeerSession& session, Response response, size_t at_most)
    : session_(session),
      response_(std::move(response)),
      sr_(new parser_type(response_)),
      at_most_(at_most)
  {
  }

  template <typename Self>
  void operator()(Self& self, bb::error_code ec = {}, std::size_t = 0)
  {
    BOOST_ASIO_CORO_REENTER(coro_)
    {
      sr_->limit(at_most_);
      BOOST_ASIO_CORO_YIELD bb::http::async_write_some(
          *session_.stream_, *sr_, std::move(self));
      self.complete(ec);
    }
  }
};
}

template <typename AsyncStream = bb::tcp_stream>
class peer_session
{
public:
  using executor_type = typename AsyncStream::executor_type;
  using request_type = bb::http::request<bb::http::string_body>;
  using response_type = bb::http::response<bb::http::string_body>;

  bb::flat_buffer buffer_;
  std::unique_ptr<AsyncStream> stream_;

  peer_session(peer_session const&) = delete;
  peer_session& operator=(peer_session const&) = delete;

public:
  peer_session() = default;
  peer_session(peer_session&& other) = default;
  peer_session& operator=(peer_session&& other) = default;

  peer_session(AsyncStream&& stream)
    : stream_(std::make_unique<AsyncStream>(std::move(stream)))
  {
  }

  auto get_executor()
  {
    return this->stream_->get_executor();
  }

  auto response(bb::http::status status,
                string_view content_type,
                string_view payload)
  {
    auto res = response_type{status, 11, payload};
    res.set(bb::http::field::content_type, content_type);
    return res;
  }

  template <typename Request = request_type, typename CompletionToken>
  auto async_receive(Request& request, CompletionToken&& token)
  {
    return net::async_compose<CompletionToken, void(bb::error_code)>(
        [&, coro = net::coroutine{}](
            auto& self, bb::error_code ec = {}, std::size_t = 0) mutable {
          BOOST_ASIO_CORO_REENTER(coro)
          {
            BOOST_ASIO_CORO_YIELD bb::http::async_read(
                *stream_, buffer_, request, std::move(self));
            self.complete(ec);
          }
        },
        token,
        *stream_);
  }

  template <typename Request = request_type, typename CompletionToken>
  auto async_receive(CompletionToken&& token)
  {
    return net::async_compose<CompletionToken, void(bb::error_code, Request)>(
        [this, req = std::make_unique<Request>(), coro = net::coroutine{}](
            auto& self, bb::error_code ec = {}, std::size_t = 0) mutable {
          BOOST_ASIO_CORO_REENTER(coro)
          {
            BOOST_ASIO_CORO_YIELD this->async_receive(*req, std::move(self));
            if (ec)
              self.complete(ec, {});
            else
              self.complete(ec, std::exchange(*req, {}));
          }
        },
        token,
        *stream_);
  }

  template <typename CompletionToken>
  auto async_receive_some(size_t at_most, CompletionToken&& token)
  {
    return net::async_compose<CompletionToken, void(bb::error_code)>(
        detail::receive_some_op{*this, at_most}, token, *stream_);
  }

  template <typename Response, typename CompletionToken>
  auto async_send(Response res, CompletionToken&& token)
  {
    return net::async_compose<CompletionToken, void(bb::error_code)>(
        [&,
         res = std::make_unique<Response>(std::move(res)),
         coro = net::coroutine{}](
            auto& self, bb::error_code ec = {}, std::size_t = 0) mutable {
          BOOST_ASIO_CORO_REENTER(coro)
          {
            BOOST_ASIO_CORO_YIELD bb::http::async_write(
                *stream_, *res, std::move(self));
            self.complete(ec);
          }
        },
        token,
        *stream_);
  }

  template <typename CompletionToken>
  auto async_send(bb::http::status status,
                  string_view content_type,
                  string_view payload,
                  CompletionToken&& token)
  {
    auto res = response_type(status, 11, payload);
    if (!content_type.empty())
      res.set(bb::http::field::content_type, content_type);
    res.prepare_payload();
    return this->async_send(std::move(res),
                            std::forward<CompletionToken>(token));
  }

  template <typename CompletionToken>
  auto async_send_some(response_type response,
                       size_t at_most,
                       CompletionToken&& token)
  {
    return net::async_compose<CompletionToken, void(bb::error_code)>(
        detail::send_some_op(*this, std::move(response), at_most),
        token,
        *stream_);
  }

  template <typename CompletionToken>
  auto async_send(bb::http::status status,
                  string_view payload,
                  CompletionToken&& token)
  {
    return this->async_send(
        status, "", payload, std::forward<CompletionToken>(token));
  }

  template <typename CompletionToken>
  auto async_reply_back(bb::http::status status, CompletionToken&& token)
  {
    return net::async_compose<CompletionToken, void(bb::error_code)>(
        detail::reply_back_op{*this, status}, token, *stream_);
  }

  /// graceful shutdown
  template <typename CompletionToken>
  auto async_read_and_shutdown(CompletionToken&& token)
  {
    return net::async_compose<CompletionToken, void(bb::error_code)>(
        [this, coro_ = net::coroutine{}](
            auto& self, bb::error_code ec = {}, size_t read_byte = {}) mutable {
          BOOST_ASIO_CORO_REENTER(coro_)
          {
            BOOST_ASIO_CORO_YIELD this->stream_->async_read_some(
                this->buffer_.prepare(40), std::move(self));
            this->buffer_.commit(read_byte);
            if (ec != net::error::eof)
            {
              // no shutdown was read
              self.complete(ec);
              return;
            }
            BOOST_ASIO_CORO_YIELD this->stream_->async_shutdown(
                std::move(self));
            bb::get_lowest_layer(*this->stream_).close();
            self.complete(ec);
          }
        },
        token,
        *stream_);
  }

  /// not so graceful shutdown
  template <typename CompletionToken>
  auto async_read_shutdown_and_close(CompletionToken&& token)
  {
    return net::async_compose<CompletionToken, void(bb::error_code)>(
        [this, coro_ = net::coroutine{}](
            auto& self, bb::error_code ec = {}, size_t read_byte = {}) mutable {
          BOOST_ASIO_CORO_REENTER(coro_)
          {
            BOOST_ASIO_CORO_YIELD this->stream_->async_read_some(
                this->buffer_.prepare(40), std::move(self));
            this->buffer_.commit(read_byte);
            if (ec == net::error::eof)
            {
              bb::get_lowest_layer(*this->stream_).close();
              self.complete({});
              return;
            }
            // no shutdown was read
            self.complete(ec);
          }
        },
        token,
        *stream_);
  }

  void close()
  {
    std::promise<void> p;
    auto f = p.get_future();
    net::dispatch(
        get_executor(),
        net::bind_executor(get_executor(), [this, p = std::move(p)]() mutable {
          bb::get_lowest_layer(*this->stream_).close();
          p.set_value();
        }));
    f.get();
  }

  void cancel()
  {
    std::promise<void> p;
    auto f = p.get_future();
    net::dispatch(
        get_executor(),
        net::bind_executor(get_executor(), [this, p = std::move(p)]() mutable {
          bb::get_lowest_layer(this->stream_)->cancel();
          p.set_value();
        }));
    f.get();
  }
};
}
