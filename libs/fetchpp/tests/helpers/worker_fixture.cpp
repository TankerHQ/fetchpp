#include "worker_fixture.hpp"

namespace test::helpers
{
worker_fixture::worker_t::worker_t()
  : ex(ioc.get_executor()),
    guard(net::make_work_guard(ex)),
    worker([this] { ioc.run(); })
{
}

auto worker_fixture::worker(int index) -> worker_t&
{
  if (auto it = workers.find(index); it != workers.end())
    return *it->second;

  return *workers.emplace(index, new worker_t).first->second;
}

worker_fixture::~worker_fixture()
{
  for (auto& [i, w] : workers)
    w->guard.reset();
  for (auto& [i, w] : workers)
  {
    w->worker.join();
    delete w;
  }
}

}
