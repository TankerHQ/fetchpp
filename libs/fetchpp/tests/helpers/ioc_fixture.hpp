#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/beast/core.hpp>

#include <fetchpp/alias/net.hpp>

#include <string>
#include <thread>

namespace test::helpers
{
struct ioc_fixture
{
  fetchpp::net::io_context ioc;
  fetchpp::net::executor ex;

  using work_guard = fetchpp::net::executor_work_guard<fetchpp::net::executor>;

  ioc_fixture()
    : ex(ioc.get_executor()),
      work(fetchpp::net::make_work_guard(ex)),
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
