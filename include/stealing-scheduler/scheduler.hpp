#pragma once

#include "./iexecutor.hpp"
#include "./itask.hpp"
#include <vector>

namespace stealing_scheduler {

struct SchedulerImpl;

class Scheduler {
  SchedulerImpl *m_impl;

public:
  explicit Scheduler(std::vector<IExecutor *> executors);
  ~Scheduler();
  void add_task(std::unique_ptr<ITask> task, IExecutor::Id id);
  void start();
  void sync();
};

} // namespace stealing_scheduler
