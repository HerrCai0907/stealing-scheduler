#pragma once

#include "./itask.hpp"
#include <functional>
#include <memory>

namespace stealing_scheduler {

class Scheduler;

class IExecutor {
public:
  using Id = IExecutor *;

  virtual ~IExecutor() = default;

  virtual Id get_id() = 0;

  virtual void start(std::function<std::unique_ptr<ITask>()> get_next_task,
                     Scheduler *scheduler) = 0;
};

} // namespace stealing_scheduler
