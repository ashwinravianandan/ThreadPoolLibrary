#ifndef _MESSAGE_PROCESSOR_H_
#define _MESSAGE_PROCESSOR_H_
#include "Queue.h"
#include "Singleton.h"
#include <exception>
#include <functional>
#include <iostream>
#include <stop_token>
#include <thread>

namespace Utils {
template <typename T> using FIFOSyncQueue = SyncronizedQueue<FIFOQueue, T>;

template <typename T>
concept Equal = requires(T &&a, T &&b) {
  a == b;
};

template <typename T, typename F>
concept ReturnsInt = requires(T &&v, F &&f) {
  { f(v) } -> std::convertible_to<unsigned int>;
};

template <typename T, typename F>
concept ReturnsHashable = requires(T &&v, F &&f) {
  {std::hash<std::decay_t<decltype(f(v))>>()(f(v))};
};

template <typename T, typename C>
concept IDType = ReturnsInt<T, C> || ReturnsHashable<T, C>;

template <typename T>
requires(std::copyable<T> || std::movable<T>) && Equal<T> struct Sentinel {
  static T getValue() {
    // default constructed object is used as sentinel
    return T{};
  }
};

struct ValueForwarder {
  template <typename T> auto operator()(T &&v) { return std::forward<T>(v); }
};

struct SequencingPolicy {};

/**
 * Ordered based on partitioning function
 */
struct WeakOrdering : public SequencingPolicy {};

/**
 * No guarantee of ordering.  All messages are processed by a pool of threads
 */
struct NoOrdering : public SequencingPolicy {};

template <typename T, int NoOfConsumers = 1, typename SeqPol = NoOrdering,
          template <typename> class Q = FIFOSyncQueue>
requires Queue<Q<T>, T> && std::convertible_to<SeqPol, SequencingPolicy>
class MessageProcessor
    : public Singleton<MessageProcessor<T, NoOfConsumers, SeqPol, Q>> {
  friend Singleton<MessageProcessor<T, NoOfConsumers, SeqPol, Q>>;

private:
  Q<T> _queue[NoOfConsumers];
  std::function<void(T &&)> _processor;
  std::jthread _threads[NoOfConsumers];
  bool _stopRequested;
  const T _sentinelVal = Sentinel<T>::getValue();

  void run(std::stop_token &token, int id = 0) {
    while (!_stopRequested || _queue[id].size() > 0) {
      try {
        auto&& val = _queue[id].dequeue();
        if (_sentinelVal == val && _queue[id].size() == 0) break;
        _processor(std::move(val));
      } catch (std::exception &ex) {
        std::cout << "Error occurred while processing message: " << ex.what();
      } catch (...) {
        std::cout << "Unknown error occurred while processing message\n";
      }
    }
  }

  MessageProcessor() = default;

public:
  ~MessageProcessor() = default;

  bool acceptWork(T& data)const
  {
     return !_stopRequested || (data == _sentinelVal);
  }

  template <typename V>
  void add(V &&data) requires std::convertible_to<V, T> &&
      std::same_as<SeqPol, NoOrdering> {
    if (acceptWork(data)) {
      _queue[0].enqueue(T(std::forward<V>(data)));
    }
  }

  /**
   * Below implementation could be an optimization to prevent hashing of ints
   */
  /*
  template <typename V, typename Callable = ValueForwarder>
  requires std::convertible_to<V, T> && std::same_as<SeqPol, WeakOrdering> &&
      ReturnsInt<V, Callable>
  void add(V &&data, Callable &&c = ValueForwarder{}) {
    if (!_stopRequested) {
      _queue[c(data) % NoOfConsumers].enqueue(T(std::forward<V>(data)));
    }
  }
  */

  template <typename V, typename Callable = ValueForwarder>
  requires std::convertible_to<V, T> && std::same_as<SeqPol, WeakOrdering> &&
      ReturnsHashable<V, Callable>
  void add(V &&data, Callable &&c = ValueForwarder{}) {
    if (acceptWork(data)) {
      std::hash<std::decay_t<decltype(c(data))>> hasher;
      _queue[hasher(c(data)) % NoOfConsumers].enqueue(T(std::forward<V>(data)));
    }
  }

  void setProcessor(const std::function<void(T &&)> &processor) {
    _processor = processor;
  }
  void start() requires std::same_as<SeqPol, NoOrdering> {
    _stopRequested = false;
    for (int i = 0; i < NoOfConsumers; i++) {
      _threads[i] = std::jthread([this](std::stop_token token) { run(token); });
    }
  }

  void start() requires std::same_as<SeqPol, WeakOrdering> {
    _stopRequested = false;
    for (int i = 0; i < NoOfConsumers; i++) {
      _threads[i] =
          std::jthread([this, i](std::stop_token token) { run(token, i); });
    }
  }
  void stop() requires std::same_as<SeqPol, NoOrdering> {
    _stopRequested = true;
    for (int i = 0; i < NoOfConsumers; i++) {
      _threads[i].request_stop();
      _queue[0].enqueue(Sentinel<T>::getValue());
    }
  }
  void stop() requires std::same_as<SeqPol, WeakOrdering> {
    _stopRequested = true;
    for (int i = 0; i < NoOfConsumers; i++) {
      _threads[i].request_stop();
      _queue[i].enqueue(Sentinel<T>::getValue());
    }
  }
};
} // namespace Utils
#endif
