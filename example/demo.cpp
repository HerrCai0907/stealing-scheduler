#include "stealing-scheduler/iexecutor.hpp"
#include "stealing-scheduler/itask.hpp"
#include "stealing-scheduler/scheduler.hpp"
#include <array>
#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <utility>

std::mutex log_mutex;

namespace stealing_scheduler::mock {

struct Task : public ITask {
  int m_cnt;
  explicit Task(int cnt) : m_cnt(cnt) {}
  std::string name() { return "Task " + std::to_string(m_cnt); }
  void run(Scheduler *scheduler, IExecutor *executor) override {
    scheduler->add_task(std::make_unique<Task>((m_cnt * 10) + 0),
                        executor->get_id());
    scheduler->add_task(std::make_unique<Task>((m_cnt * 10) + 1),
                        executor->get_id());
    std::this_thread::sleep_for(std::chrono::seconds{1});
  }
};

struct Executor : public IExecutor {
  std::thread m_th;
  std::function<void()> m_fn;

  Id get_id() override { return this; }
  void start(std::function<std::unique_ptr<ITask>()> get_next_task,
             Scheduler *scheduler) override {
    m_th = std::thread(
        [this, get_next_task = std::move(get_next_task), scheduler]() -> void {
          for (;;) {
            std::unique_ptr<ITask> task = get_next_task();
            Task *demo_task = static_cast<Task *>(task.get());
            {
              std::lock_guard<std::mutex> lock{log_mutex};
              std::cout << "Executor [" << m_th.get_id() << "] is running on "
                        << demo_task->name() << "\n";
            }
            task->run(scheduler, this);
          }
        });
  }
};
} // namespace stealing_scheduler::mock

int main() {
  using namespace stealing_scheduler::mock;
  using namespace stealing_scheduler;

  std::array<Executor, 4> executors{};
  std::vector<IExecutor *> executors_ptr;
  for (Executor &executor : executors) {
    executors_ptr.push_back(&executor);
  }
  Scheduler scheduler{std::move(executors_ptr)};
  scheduler.add_task(std::make_unique<Task>(1), executors[0].get_id());
  scheduler.start();

  for (Executor &e : executors) {
    e.m_th.join();
  }
  return 0;
}
