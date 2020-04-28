#pragma once

#include <memory>
#include <queue>

namespace fetchpp::detail
{
class session_work_queue
{
public:
  struct work
  {
    virtual ~work() = 0;

    virtual void operator()(session_work_queue&) = 0;
  };
  using work_ptr = std::unique_ptr<work>;

  void advance();
  void push(work_ptr);
  void consume();
  std::size_t size() const;

private:
  std::queue<work_ptr> items_;
};

}
