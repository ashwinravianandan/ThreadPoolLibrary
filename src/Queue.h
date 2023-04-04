#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <algorithm>
#include <concepts>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>

namespace Utils {

template <typename T, typename U>
concept Queue = (std::copyable<U> || std::movable<U>)&&requires(T &&x, U &&u) {
  {x.enqueue(std::forward<U>(u))};
  { x.dequeue() } -> std::convertible_to<U>;
  { x.empty() } -> std::same_as<bool>;
  { x.size() } -> std::convertible_to<size_t>;
};

template <typename T>
requires std::copyable<T> || std::movable<T>
class FIFOQueue {
private:
  std::deque<T> _queue;

public:
  template <typename U>
  void enqueue(U &&val) { _queue.push_back(std::forward<U>(val)); }
  T dequeue() {
    if (_queue.empty())
      throw std::runtime_error("Queue is empty");
    T val = std::move(_queue.front());
    _queue.pop_front();
    return std::move(val);
  }
  auto empty() { return _queue.empty(); }
  auto size() { return _queue.size(); }
};

template <typename T>
requires std::copyable<T> || std::movable<T>
class NonBufferingQueue {
private:
  std::unique_ptr<T> _data;
  std::condition_variable _emptyQ;
  std::condition_variable _fullQ;
  std::mutex _mtx;

public:
  template <typename U>
  void enqueue(U &&val) {
    std::unique_lock<std::mutex> lock{_mtx};
    _fullQ.wait(lock, [this]() { return !_data; });
    _data = std::make_unique<T>(std::forward<U>(val));
    _emptyQ.notify_one();
  }
  T dequeue() {
    std::unique_lock<std::mutex> lock{_mtx};
    _emptyQ.wait(lock, [this]() { return static_cast<bool>(_data); });
    T val = *_data;
    _data.reset(nullptr);
    _fullQ.notify_one();
    return std::move(val);
  }
  bool empty() const { return !_data; }
  size_t size() const { return _data ? 1 : 0; }
};

template <template <typename V> class Q, typename T>
requires(std::copyable<T> || std::movable<T>) &&
    Queue<Q<T>, T> class SyncronizedQueue : public Q<T> {
private:
  using Base = Q<T>;
  std::mutex _mtx;
  std::condition_variable _emptyQ;

public:
  template <typename U>
  void enqueue(U &&val) {
    std::lock_guard<std::mutex> lock{_mtx};
    Base::enqueue(std::forward<U>(val));
    _emptyQ.notify_one();
  }
  T dequeue() {
    std::unique_lock<std::mutex> lock{_mtx};
    _emptyQ.wait(lock, [this]() { return !Base::empty(); });
    return std::move(Base::dequeue());
  }

  using Base::empty;
  using Base::size;
};
} // namespace Utils

#endif
