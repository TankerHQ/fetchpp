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

  using work_guard = fetchpp::net::executor_work_guard<
      fetchpp::net::io_context::executor_type>;

  ioc_fixture()
    : work(fetchpp::net::make_work_guard(ioc)), worker([this]() { ioc.run(); })
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
