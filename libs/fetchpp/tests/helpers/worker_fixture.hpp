#pragma once

#include <boost/asio/executor.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>

#include <map>
#include <string>
#include <thread>

namespace test::helpers
{
namespace net = boost::asio;

struct worker_fixture
{
  using work_guard = net::executor_work_guard<net::executor>;
  struct worker_t
  {
    net::io_context ioc;
    net::executor ex;
    work_guard guard;
    std::thread worker;
    worker_t();
  };

  auto worker(int index = 0) -> worker_t&;

  virtual ~worker_fixture();

private:
  std::map<int, worker_t*> workers;
};
}
