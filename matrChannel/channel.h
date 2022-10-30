#pragma once
#include <condition_variable>
#include <mutex>
#include <queue>

std::mutex write_lock;
std::mutex read_lock;

std::condition_variable write_variable;
std::condition_variable read_variable;

template <class T>
class Channel {
 private:
  int size;
  bool can_write = true;
  bool can_read = true;
  bool is_closed = false;
  std::queue<T> queue;
  void Check() {
    can_write = queue.size() < size;
    can_read = !queue.empty();
  }

 public:
  explicit Channel(int size_) {
    size = size_;
    Check();
  }

  void Send(T value) {
    if (is_closed) {
      throw std::runtime_error("Access is denied.");
    }
    std::unique_lock<std::mutex> locker(write_lock);
    while (!can_write) {
      write_variable.wait(locker);
    }
    queue.push(value);
    Check();
    write_variable.notify_one();
    read_variable.notify_one();
  }

  void Close() { is_closed = true; }

  std::pair<T, bool> Recv() {
    if (is_closed && !can_read) {
      return {T(), false};
    }
    std::unique_lock<std::mutex> locker(read_lock);
    while (!can_read) {
      write_variable.wait(locker);
    }
    T head = queue.front();
    queue.pop();
    Check();
    write_variable.notify_one();
    read_variable.notify_one();
    return {head, false};
  }
};
