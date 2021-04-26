#pragma once

#include <boost/asio/executor.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>

#include <memory>
#include <queue>

#include <fetchpp/alias/net.hpp>

namespace fetchpp::detail
{
struct task
{
  virtual ~task() = 0;
  using ptr_t = std::unique_ptr<task>;
  virtual void run() = 0;
  virtual void cancel() = 0;
};

struct session_base
{
  session_base(session_base const&) = delete;
  session_base& operator=(session_base const&) = delete;

  session_base(session_base&&) = delete;
  session_base& operator=(session_base&&) = delete;
  session_base(net::executor default_ex);

  using internal_executor_type = net::strand<net::executor>;
  using default_executor_type =
      typename internal_executor_type::inner_executor_type;

  auto get_default_executor() const -> default_executor_type;
  auto get_internal_executor() const -> internal_executor_type;

  bool running() const;

  std::size_t pending_tasks() const;
  bool has_tasks() const;
  void push_task(detail::task::ptr_t t);
  void process_task();
  void pop_task();
  void cancel_all_tasks();
  void advance_task();

  template <typename CompletionToken>
  auto async_wait_for_tasks_cancellation(CompletionToken&& token)
  {
    // this should be more than long enough to let everything complete
    this->timer_.expires_after(std::chrono::milliseconds(500));
    return this->timer_.async_wait(std::forward<CompletionToken>(token));
  }

  void set_running(bool new_state);

  // we don't want to pass as an io object
  auto get_executor() -> internal_executor_type = delete;

private:
  internal_executor_type strand_;
  net::steady_timer timer_;
  std::queue<detail::task::ptr_t> tasks_;
  bool is_running_ = true;
};

}
