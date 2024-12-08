#include "stealing-scheduler/scheduler.hpp"
#include "stealing-scheduler/iexecutor.hpp"
#include "stealing-scheduler/itask.hpp"
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <random>
#include <utility>

namespace stealing_scheduler {

namespace {

// task queue support add, pop and steal task.
class TaskQueue {
  std::mutex m_mutex{};
  std::deque<std::unique_ptr<ITask>> m_queue{};

public:
  // add task to the queue
  void push(std::unique_ptr<ITask> task);
  // consume task from the queue
  std::unique_ptr<ITask> pop();
  // steal task from the queue
  std::unique_ptr<ITask> steal();
};

void TaskQueue::push(std::unique_ptr<ITask> task) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_queue.push_back(std::move(task));
}
std::unique_ptr<ITask> TaskQueue::pop() {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_queue.empty()) {
    return nullptr;
  }
  std::unique_ptr<ITask> ret = std::move(m_queue.front());
  m_queue.pop_front();
  return ret;
}
std::unique_ptr<ITask> TaskQueue::steal() {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_queue.empty()) {
    return nullptr;
  }
  std::unique_ptr<ITask> ret = std::move(m_queue.back());
  m_queue.pop_back();
  return ret;
}

struct ExecutorContext {
  explicit ExecutorContext(IExecutor *m_executor)
      : m_executor(m_executor), m_queue() {}

  IExecutor *m_executor;
  TaskQueue m_queue;

  std::unique_ptr<ITask> get_next_task(SchedulerImpl *scheduler);
};

} // namespace

struct SchedulerImpl {
  std::vector<std::unique_ptr<ExecutorContext>> m_contexts{};
  std::map<IExecutor::Id, ExecutorContext *> m_m_contexts_map{};
  std::mt19937 m_rng{std::random_device{}()};
};

namespace {

std::unique_ptr<ITask> ExecutorContext::get_next_task(SchedulerImpl *impl) {
  // priority 1: consume from self queue
  std::unique_ptr<ITask> task = m_queue.pop();
  if (task != nullptr) {
    return task;
  }
  for (;;) {
    // priority 2: consume from random selected other queue.
    const size_t size = impl->m_contexts.size();
    std::uniform_int_distribution<size_t> m_dist{0U, size - 1};
    const size_t offset = m_dist(impl->m_rng);
    for (size_t i = 0; i < size; i++) {
      std::unique_ptr<ITask> stolen_task =
          impl->m_contexts[(i + offset) % size]->m_queue.steal();
      if (stolen_task != nullptr) {
        return stolen_task;
      }
    }
  }
}

} // namespace

Scheduler::Scheduler(std::vector<IExecutor *> executors)
    : m_impl(new SchedulerImpl()) {
  for (IExecutor *executor : executors) {
    std::unique_ptr<ExecutorContext> executor_impl =
        std::make_unique<ExecutorContext>(executor);
    ExecutorContext *executor_impl_ptr = executor_impl.get();
    m_impl->m_contexts.push_back(std::move(executor_impl));
    m_impl->m_m_contexts_map.insert(
        std::make_pair(executor->get_id(), executor_impl_ptr));
  }
}

Scheduler::~Scheduler() { delete m_impl; }

void Scheduler::add_task(std::unique_ptr<ITask> task, IExecutor::Id id) {
  m_impl->m_m_contexts_map.at(id)->m_queue.push(std::move(task));
}

void Scheduler::start() {
  for (auto &context : m_impl->m_contexts) {
    context->m_executor->start(
        [context = context.get(), this]() -> std::unique_ptr<ITask> {
          return context->get_next_task(m_impl);
        },
        this);
  }
}

void Scheduler::sync() {
  // todo
}

} // namespace stealing_scheduler
