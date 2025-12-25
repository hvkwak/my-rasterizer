#ifndef QUEUE_H
#define QUEUE_H
#include <queue>
#include <mutex>
#include <condition_variable>

/**
 * @brief Thread-safe queue that sleeps when empty and wakes when work is available.
 *
 * This queue blocks on pop() when empty, avoiding CPU waste while waiting.
 * Threads can be signaled to stop waiting via stop().
 */
template <class T>
class Queue {
public:

  /**
   * @brief Push an item to the queue and wake up one waiting thread.
   * @param v The value to push
   */
  void push(T v) {
    {
      std::lock_guard<std::mutex> lk(m_);
      q_.push(std::move(v));
    }
    cv_.notify_one();
  }

  /**
   * @brief Pop an item from the queue, blocking if empty.
   * If the queue is empty, this thread will sleep until:
   * - An item is pushed (returns true with the item)
   * - stop() is called (returns false if queue remains empty)
   *
   * @param out Reference to store the popped value
   * @return true if an item was successfully popped, false if stopped
   */
  bool pop(T& out) {
    std::unique_lock<std::mutex> lk(m_);
    // Wait until queue has items or stop is signaled
    // Internally: unlocks mutex before sleeping, relocks when awakened
    cv_.wait(lk, [&] { return stop_ || !q_.empty(); });

    // If stop signaled and queue is empty, exit
    if (stop_ && q_.empty()) return false;

    out = std::move(q_.front());
    q_.pop();
    return true;
  }

  /**
   * @brief Try to pop without blocking.
   * @param out Reference to store the popped value
   * @return true if item was popped, false if queue was empty
   */
  bool try_pop(T& out) {
    std::lock_guard<std::mutex> lk(m_);
    if (q_.empty()) return false;

    out = std::move(q_.front());
    q_.pop();
    return true;
  }

  /**
   * @brief Signal all threads to stop waiting and exit.
   *
   * Sets the stop flag and wakes all sleeping threads.
   * Subsequent pop() calls will return false once the queue is empty.
   */
  void stop() {
    {
      std::lock_guard<std::mutex> lk(m_);
      stop_ = true;
    }
    cv_.notify_all();
  }

private:
  std::queue<T> q_;
  std::mutex m_;
  std::condition_variable cv_;
  bool stop_ = false;
};

#endif // QUEUE_H
