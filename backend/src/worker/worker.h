#pragma once
#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>

namespace Crown {

struct TaskQueue {
  std::queue<std::function<void()>> queue_;
  std::mutex mutex_;
  std::condition_variable cv_push_;
  std::condition_variable cv_pop_;
  bool running_ = true;
  size_t maxSize_;

  explicit TaskQueue(size_t maxSize = 1024) : maxSize_(maxSize) {}

  bool push(std::function<void()> fn) {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_push_.wait(lock, [&] { return queue_.size() < maxSize_ || !running_; });
    if (!running_)
      return false;
    queue_.push(std::move(fn));
    cv_pop_.notify_one();
    return true;
  }

  std::function<void()> pop() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_pop_.wait(lock, [&] { return !queue_.empty() || !running_; });
    if (queue_.empty())
      return nullptr;
    auto task = std::move(queue_.front());
    queue_.pop();
    cv_push_.notify_one();
    return task;
  }

  void shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    cv_push_.notify_all();
    cv_pop_.notify_all();
  }
};

struct WorkerPool {
  TaskQueue tasks_;
  std::vector<std::thread> threads_;

  explicit WorkerPool(size_t n = 1) {
    threads_.reserve(n);
    for (size_t i = 0; i < n; ++i) {
      threads_.emplace_back([this] {
        while (auto task = tasks_.pop()) {
          try {
            task();
          } catch (const std::exception &e) {
            std::cerr << "[WorkerPool] task error: " << e.what() << "\n";
          } catch (...) {
            std::cerr << "[WorkerPool] unknown task error\n";
          }
        }
      });
    }
  }

  ~WorkerPool() {
    tasks_.shutdown();
    for (auto &t : threads_) {
      if (t.joinable())
        t.join();
    }
  }

  void submit(std::function<void()> fn) { tasks_.push(std::move(fn)); }

  WorkerPool(const WorkerPool &) = delete;
  WorkerPool &operator=(const WorkerPool &) = delete;
};
} // namespace Crown
