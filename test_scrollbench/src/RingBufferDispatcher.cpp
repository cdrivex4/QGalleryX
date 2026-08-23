#include "RingBufferDispatcher.h"

RingBufferDispatcher::LockFreeRing::LockFreeRing(size_t cap)
    : buffer(cap), capacity(cap) {}

bool RingBufferDispatcher::LockFreeRing::push(const DispatchEntry &item) {
  while (spinlock.test_and_set(std::memory_order_acquire)) {
  }

  size_t currentTail = tail.load(std::memory_order_relaxed);
  size_t currentHead = head.load(std::memory_order_relaxed);

  if ((currentTail + 1) % capacity == currentHead) {
    spinlock.clear(std::memory_order_release);
    return false; // Ring full
  }

  size_t index = currentTail % capacity;
  buffer[index].entry = item;
  tail.store((currentTail + 1) % capacity, std::memory_order_relaxed);

  spinlock.clear(std::memory_order_release);
  return true;
}

bool RingBufferDispatcher::LockFreeRing::pop(DispatchEntry &item) {
  while (spinlock.test_and_set(std::memory_order_acquire)) {
  }

  size_t currentHead = head.load(std::memory_order_relaxed);
  size_t currentTail = tail.load(std::memory_order_relaxed);

  if (currentHead == currentTail) {
    spinlock.clear(std::memory_order_release);
    return false; // Ring empty
  }

  size_t index = currentHead % capacity;
  item = std::move(buffer[index].entry);
  buffer[index].entry.task = nullptr;
  buffer[index].entry.key.clear();
  head.store((currentHead + 1) % capacity, std::memory_order_relaxed);

  spinlock.clear(std::memory_order_release);
  return true;
}

bool RingBufferDispatcher::LockFreeRing::popLifo(DispatchEntry &item) {
  while (spinlock.test_and_set(std::memory_order_acquire)) {
  }

  size_t currentHead = head.load(std::memory_order_relaxed);
  size_t currentTail = tail.load(std::memory_order_relaxed);

  if (currentHead == currentTail) {
    spinlock.clear(std::memory_order_release);
    return false; // Ring empty
  }

  // LIFO: Take from the tail (most recently added)
  size_t newTail = (currentTail == 0) ? (capacity - 1) : (currentTail - 1);
  item = std::move(buffer[newTail].entry);
  buffer[newTail].entry.task = nullptr;
  buffer[newTail].entry.key.clear();
  tail.store(newTail, std::memory_order_relaxed);

  spinlock.clear(std::memory_order_release);
  return true;
}

void RingBufferDispatcher::LockFreeRing::clear() {
  while (spinlock.test_and_set(std::memory_order_acquire)) {
  }
  head.store(0, std::memory_order_relaxed);
  tail.store(0, std::memory_order_relaxed);
  // Destroy std::functions cleanly to prevent memory leak
  for (auto &slot : buffer) {
    slot.entry.task = nullptr;
    slot.entry.key.clear();
  }
  spinlock.clear(std::memory_order_release);
}


size_t RingBufferDispatcher::LockFreeRing::size() const {
  size_t currentHead = head.load(std::memory_order_relaxed);
  size_t currentTail = tail.load(std::memory_order_relaxed);
  if (currentTail >= currentHead) {
    return currentTail - currentHead;
  }
  return capacity - (currentHead - currentTail);
}

RingBufferDispatcher::RingBufferDispatcher(size_t capacity)
    : m_ring0(capacity), m_ring1(capacity), m_ring2(capacity) {}

bool RingBufferDispatcher::push(Task task, RingPriority priority,
                                const QString &key) {
  DispatchEntry entry;
  entry.task = task;
  entry.key = key;
  entry.priority = priority;
  entry.sequence = m_sequence.fetch_add(1, std::memory_order_relaxed);

  switch (priority) {
  case Ring0_Immediate:
    return m_ring0.push(entry);
  case Ring1_Lookahead:
    return m_ring1.push(entry);
  case Ring2_Precache:
    return m_ring2.push(entry);
  }
  return false;
}

bool RingBufferDispatcher::pop(DispatchEntry &outEntry, int governorMode, bool allowBackground) {
  // governorMode: 0 = FIFO, 1 = LIFO, 2 = Adaptive (LIFO for Ring0, FIFO for others)
  
  // Strict Ring Priority: Ring 0 (Immediate) > Ring 1 (Lookahead) > Ring 2 (Precache)
  if (governorMode == 1 || governorMode == 2) {
    if (m_ring0.popLifo(outEntry))
      return true;
  } else {
    if (m_ring0.pop(outEntry))
      return true;
  }

  // If background is paused, do not touch Ring 1 or Ring 2 (leaves them safely in place)
  if (!allowBackground) {
    return false;
  }

  if (governorMode == 1) {
    if (m_ring1.popLifo(outEntry))
      return true;
    if (m_ring2.popLifo(outEntry))
      return true;
  } else {
    if (m_ring1.pop(outEntry))
      return true;
    if (m_ring2.pop(outEntry))
      return true;
  }

  return false;
}


void RingBufferDispatcher::clear() {
  m_ring0.clear();
  m_ring1.clear();
  m_ring2.clear();
}

size_t RingBufferDispatcher::size(RingPriority priority) const {
  switch (priority) {
  case Ring0_Immediate:
    return m_ring0.size();
  case Ring1_Lookahead:
    return m_ring1.size();
  case Ring2_Precache:
    return m_ring2.size();
  }
  return 0;
}

bool RingBufferDispatcher::isEmpty() const {
  return (m_ring0.size() == 0 && m_ring1.size() == 0 && m_ring2.size() == 0);
}
