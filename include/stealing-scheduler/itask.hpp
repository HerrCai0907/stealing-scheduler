#pragma once

namespace stealing_scheduler {

class Scheduler;
class IExecutor;

class ITask {
public:
  virtual ~ITask() = default;

  virtual void run(Scheduler *scheduler, IExecutor *executor) = 0;
};

} // namespace stealing_scheduler
