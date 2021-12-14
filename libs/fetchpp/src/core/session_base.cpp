#include <fetchpp/core/detail/session_base.hpp>

namespace fetchpp::detail
{
task::~task() = default;

session_base::session_base(net::any_io_executor default_ex)
  : strand_(default_ex), timer_(strand_)
{
}

auto session_base::get_default_executor() const -> default_executor_type
{
  return strand_.get_inner_executor();
}

auto session_base::get_internal_executor() const -> internal_executor_type
{
  return strand_;
}

std::size_t session_base::pending_tasks() const
{
  return tasks_.size();
}

bool session_base::has_tasks() const
{
  return pending_tasks() > 0;
}

bool session_base::running() const
{
  return is_running_;
}

void session_base::push_task(detail::task::ptr_t t)
{
  tasks_.push(std::move(t));
}

void session_base::process_task()
{
  BOOST_ASSERT(get_internal_executor().running_in_this_thread());
  if (has_tasks())
    tasks_.front()->run();
}

void session_base::pop_task()
{
  BOOST_ASSERT(get_internal_executor().running_in_this_thread());
  BOOST_ASSERT(has_tasks());
  tasks_.pop();
}

void session_base::cancel_all_tasks()
{
  while (this->has_tasks())
  {
    tasks_.front()->cancel();
    pop_task();
  }
  // we notify the timer that we have finished
  this->timer_.cancel();
}

void session_base::advance_task()
{
  this->pop_task();
  if (this->running())
    this->process_task();
  else
    this->cancel_all_tasks();
}

void session_base::set_running(bool new_state)
{
  is_running_ = new_state;
}

}
