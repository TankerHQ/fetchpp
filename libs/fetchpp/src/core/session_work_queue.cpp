#include <fetchpp/core/detail/session_work_queue.hpp>

namespace fetchpp::detail
{
session_work_queue::work::~work() = default;

void session_work_queue::advance()
{
  if (!items_.empty())
    (*items_.front())(*this);
}

void session_work_queue::push(session_work_queue::work_ptr item)
{
  items_.emplace(std::move(item));
  if (items_.size() == 1)
    (*items_.front())(*this);
}

void session_work_queue::consume()
{
  items_.pop();
  advance();
}

std::size_t session_work_queue::size() const
{
  return items_.size();
}
}
