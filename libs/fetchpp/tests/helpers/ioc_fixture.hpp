#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/beast/core.hpp>

#include <string>
#include <thread>

namespace test::helpers
{
namespace net = boost::asio;

struct ioc_fixture
{
  net::io_context ioc;
  net::executor ex;

  using work_guard = net::executor_work_guard<net::executor>;

  ioc_fixture()
    : ex(ioc.get_executor()),
      work(net::make_work_guard(ex)),
      worker([this]() { ioc.run(); })
  {
  }
  virtual ~ioc_fixture()
  {
    work.reset();
    worker.join();
  }

private:
  work_guard work;
  std::thread worker;
};

}
