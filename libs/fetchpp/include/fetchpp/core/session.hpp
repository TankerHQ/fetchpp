#pragma once

#include <fetchpp/core/basic_transport.hpp>
#include <fetchpp/core/detail/coroutine.hpp>
#include <fetchpp/core/process_one.hpp>
#include <fetchpp/core/ssl_transport.hpp>
#include <fetchpp/core/tcp_transport.hpp>
#include <fetchpp/core/tunnel_transport.hpp>

#include <fetchpp/core/detail/session_base.hpp>

#include <fetchpp/net/make_dispatch.hpp>
#include <fetchpp/net/make_post.hpp>

#include <boost/asio/compose.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core/error.hpp>

#include <fetchpp/alias/beast.hpp>
#include <fetchpp/alias/error_code.hpp>
#include <fetchpp/alias/net.hpp>
#include <fetchpp/alias/tcp.hpp>

#include <chrono>

namespace fetchpp
{
namespace detail
{
template <typename TaskState>
struct process_queue_op
{
  TaskState& task_;
  error_code last_ec_ = {};
  net::coroutine coro_ = {};
  bool first_round_ = true;

  using executor_type =
      typename TaskState::session_type::internal_executor_type;
  auto get_executor() const
  {
    return task_.session_.get_internal_executor();
  }

  template <typename... Args>
  void complete(Args&&... args)
  {
    auto h_ex = net::get_associated_executor(
        task_.handler_, task_.session_.get_default_executor());
    net::post(h_ex,
              beast::bind_front_handler(std::move(task_.handler_),
                                        std::forward<Args>(args)...));

    net::post(task_.session_.get_internal_executor(),
              [&sess = task_.session_] { sess.advance_task(); });
  }

  bool still_running()
  {
    return task_.session_.running();
  }

  auto& session()
  {
    return task_.session_;
  }

  decltype(auto) transport()

  {
    return session().transport();
  }

  auto get_executor()
  {
    return session().get_internal_executor();
  }

  void operator()(error_code ec = {})
  {
    BOOST_ASSERT(get_executor().running_in_this_thread());
    if (!still_running() || ec == net::error::operation_aborted)
    {
      complete(net::error::operation_aborted);
      return;
    }

    FETCHPP_REENTER(coro_)
    {
      if (!transport().is_open() && still_running())
      {
        FETCHPP_YIELD task_.session_.async_transport_connect(std::move(*this));
        if (ec)
        {
          session().set_running(false);
          complete(ec);
          return;
        }
      }

      FETCHPP_YIELD async_process_one(session().transport_,
                                      task_.request_,
                                      task_.response_,
                                      std::move(*this));

      // If an error occurred and this is a reused connection, silence the
      // error and try again
      if (ec)
      {
        last_ec_ = ec;
        FETCHPP_YIELD
        session().transport_.async_close(std::move(*this));
        ec = last_ec_;
        if (first_round_)
        {
          // we reset the coroutine state to try a re-connection
          coro_ = {};
          first_round_ = false;
          net::post(beast::bind_front_handler(std::move(*this), error_code{}));
          return;
        }
        else
        {
          session().set_running(false);
        }
      }

      complete(ec);
    }
  }
};
template <typename TaskState>
process_queue_op(TaskState&) -> process_queue_op<TaskState>;

template <typename Session,
          typename Request,
          typename Response,
          typename Handler>
struct task_state
{
  using session_type = Session;
  session_type& session_;
  Request& request_;
  Response& response_;
  Handler handler_;
};
template <typename Session,
          typename Request,
          typename Response,
          typename Handler>
task_state(Session&, Request&, Response&, Handler)
    -> task_state<Session, Request, Response, Handler>;

template <typename TaskState>
auto make_task(TaskState&& state)
{
  struct session_task : detail::task
  {
    TaskState state_;

    session_task(TaskState taskState) : state_(std::move(taskState))
    {
    }

    void run() override
    {
      BOOST_ASSERT(
          state_.session_.get_internal_executor().running_in_this_thread());

      process_queue_op op{state_};
      op();
    }

    void cancel() override
    {
      BOOST_ASSERT(
          state_.session_.get_internal_executor().running_in_this_thread());
      net::make_post(
          beast::bind_front_handler(std::move(state_.handler_),
                                    boost::asio::error::operation_aborted),
          state_.session_.get_default_executor());
    }
  };
  return std::make_unique<session_task>(std::move(state));
}

template <typename Session,
          typename Request,
          typename Response,
          typename Handler>
struct push_op
{
  Session& session_;
  Request& request_;
  Response& response_;
  Handler handler_;
  net::coroutine coro_ = {};

  push_op(Session& sess, Request& req, Response& res, Handler&& h)
    : session_(sess), request_(req), response_(res), handler_(std::move(h))
  {
  }

  void operator()()
  {
    if (!session_.running())
    {
      net::make_post(beast::bind_front_handler(std::move(handler_),
                                               net::error::operation_aborted),
                     session_.get_default_executor());
      return;
    }
    BOOST_ASSERT(session_.get_internal_executor().running_in_this_thread());
    session_.push_task(detail::make_task(detail::task_state{
        session_, request_, response_, std::move(handler_)}));
    if (session_.pending_tasks() == 1)
      net::post(session_.get_internal_executor(),
                [&sess = session_]() { sess.process_task(); });
  }

  using executor_type = typename Session::internal_executor_type;
  auto get_executor() const
  {
    return session_.get_internal_executor();
  }
};

template <typename Session, typename Handler>
struct start_op
{
  Session& session_;
  Handler handler_;
  net::coroutine coro_ = {};

  void operator()(error_code ec = {})
  {
    FETCHPP_REENTER(coro_)
    {

      BOOST_ASSERT(session_.get_internal_executor().running_in_this_thread());
      session_.set_running(true);
      FETCHPP_YIELD session_.async_transport_connect(std::move(*this));
      net::make_dispatch(beast::bind_front_handler(std::move(handler_), ec),
                         session_.get_default_executor());
    }
  }

  using executor_type = typename Session::internal_executor_type;
  auto get_executor() const
  {
    return session_.get_internal_executor();
  }
};
template <typename Session, typename Handler>
start_op(Session& session, Handler) -> start_op<Session, Handler>;

template <typename Session, typename Handler>
struct stop_op
{
  struct state
  {
    GracefulShutdown gr_;
    Session& session_;
    Handler handler_;
  };
  std::unique_ptr<state> state_;
  net::coroutine coro_ = {};

  stop_op(GracefulShutdown gr, Session& sess, Handler&& h)
    : state_(std::unique_ptr<state>(new state{gr, sess, std::move(h)}))
  {
  }

  void operator()(error_code ec = {})
  {
    FETCHPP_REENTER(coro_)
    {
      BOOST_ASSERT(
          state_->session_.get_internal_executor().running_in_this_thread());
      state_->session_.set_running(false);
      FETCHPP_YIELD state_->session_.async_transport_close(state_->gr_,
                                                           std::move(*this));
      if (state_->session_.has_tasks())
      {
        FETCHPP_YIELD state_->session_.async_wait_for_tasks_cancellation(
            std::move(*this));
        // we cancel the timer when all tasks have been cancelled
        if (ec == net::error::operation_aborted)
          ec = {};
        // the timer has run out, not all tasks may have been cancelled
        else if (!ec)
          ec = net::error::timed_out;
      }
      net::make_dispatch(
          beast::bind_front_handler(std::move(state_->handler_), ec),
          state_->session_.get_default_executor());
    }
  }

  using executor_type = typename Session::internal_executor_type;
  auto get_executor() const
  {
    return state_->session_.get_internal_executor();
  }
};
}

template <typename Endpoint, typename AsyncTransport>
class session : public detail::session_base
{
  template <typename TaskState>
  friend struct detail::process_queue_op;
  template <typename Session, typename Handler>
  friend struct detail::start_op;
  template <typename Session, typename Handler>
  friend struct detail::stop_op;

public:
  session(session const&) = delete;
  session& operator=(session const&) = delete;

  session(session&&) = delete;
  session& operator=(session&&) = delete;

  using session_type = session<Endpoint, AsyncTransport>;
  using transport_type = AsyncTransport;
  using next_layer_type = typename transport_type::next_layer_type;
  using buffer_type = typename transport_type::buffer_type;
  using endpoint_type = Endpoint;

  template <typename... Args>
  session(endpoint_type endpoint, net::executor ex, Args&&... args)
    : session_base{ex},
      endpoint_(std::move(endpoint)),
      transport_(ex, std::forward<Args>(args)...)
  {
  }

  template <typename CompletionToken>
  auto async_start(CompletionToken&& token)
  {
    auto launch = [](auto&& handler, session_type* session) mutable {
      auto op =
          detail::start_op{*session, std::forward<decltype(handler)>(handler)};
      net::dispatch(std::move(op));
    };
    return net::async_initiate<CompletionToken, void(error_code)>(
        std::move(launch), token, this);
  }

  template <typename Response, typename Request, typename CompletionToken>
  auto push_request(Request& req, Response& res, CompletionToken&& token)
  {
    auto launch =
        [](auto&& handler, session_type* session, Request& req, Response& res) {
          auto op = detail::push_op(
              *session, req, res, std::forward<decltype(handler)>(handler));
          net::dispatch(std::move(op));
        };
    return net::async_initiate<CompletionToken, void(error_code)>(
        std::move(launch), token, this, std::ref(req), std::ref(res));
  }

  template <typename CompletionToken>
  auto async_stop(GracefulShutdown graceful, CompletionToken&& token)
  {
    auto launch =
        [](auto&& handler, GracefulShutdown gr, session_type* session) mutable {
          auto op = detail::stop_op{gr, *session, std::move(handler)};
          net::dispatch(std::move(op));
        };
    return net::async_initiate<CompletionToken, void(error_code)>(
        std::move(launch), token, graceful, this);
  }

  template <typename CompletionToken>
  auto async_stop(CompletionToken&& token)
  {
    return async_stop(GracefulShutdown::Yes,
                      std::forward<CompletionToken>(token));
  }

  auto const& endpoint() const
  {
    return endpoint_;
  }

private:
  template <typename CompletionToken>
  auto async_transport_connect(CompletionToken&& token)
  {
    return this->transport().async_connect(
        endpoint_, std::forward<CompletionToken>(token));
  }

  template <typename CompletionToken>
  auto async_transport_close(GracefulShutdown gr, CompletionToken&& token)
  {
    return this->transport().async_close(gr,
                                         std::forward<CompletionToken>(token));
  }

  transport_type& transport()
  {
    return transport_;
  }

  endpoint_type endpoint_;
  transport_type transport_;
};

template <typename... Args>
session(secure_endpoint, net::executor ex, std::chrono::nanoseconds, Args&&...)
    -> session<secure_endpoint, ssl_async_transport>;

template <typename... Args>
session(plain_endpoint, net::executor ex, std::chrono::nanoseconds, Args&&...)
    -> session<plain_endpoint, tcp_async_transport>;

template <typename... Args>
session(tunnel_endpoint, net::executor ex, std::chrono::nanoseconds, Args&&...)
    -> session<tunnel_endpoint, tunnel_async_transport>;
}
