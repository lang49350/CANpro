#ifndef SPSCQUEUE_H
#define SPSCQUEUE_H

#include <QByteArray>
#include <atomic>
#include <array>
#include <cstdint>

// SPSC Lock-free Queue Template
template <size_t Capacity>
class SpscQByteArrayQueue final {
public:
    bool push(QByteArray &&data) {
        const uint32_t head = m_head.load(std::memory_order_relaxed);
        const uint32_t next = inc(head);
        if (next == m_tail.load(std::memory_order_acquire)) {
            return false;
        }
        m_ring[head] = std::move(data);
        m_head.store(next, std::memory_order_release);
        return true;
    }

    bool pop(QByteArray &out) {
        const uint32_t tail = m_tail.load(std::memory_order_relaxed);
        if (tail == m_head.load(std::memory_order_acquire)) {
            return false;
        }
        out = std::move(m_ring[tail]);
        m_ring[tail].clear();
        m_tail.store(inc(tail), std::memory_order_release);
        return true;
    }

    void clear() {
        QByteArray tmp;
        while (pop(tmp)) {
        }
    }

private:
    static constexpr uint32_t kCapacity = static_cast<uint32_t>(Capacity);

    static uint32_t inc(uint32_t v) {
        return (v + 1) % kCapacity;
    }

    std::array<QByteArray, Capacity> m_ring{};
    std::atomic<uint32_t> m_head{0};
    std::atomic<uint32_t> m_tail{0};
};

#endif // SPSCQUEUE_H
