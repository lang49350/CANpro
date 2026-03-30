/*
 * ESP32-S3 ZCAN Gateway - PSRAM 环形缓冲区实现
 * 单生产者单消费者，原子 head/tail，用于 CAN→USB 不丢包路径
 * 
 * 设计要点：
 * - 使用PSRAM分配大缓冲区（2MB），可缓冲约10秒高速数据
 * - 单生产者（CAN RX任务）单消费者（USB TX任务）模型
 * - 使用原子操作保证双核安全（Core 1写，Core 0读）
 * - 环形缓冲区算法：head指向下一个写入位置，tail指向下一个读取位置
 * - 满条件：(head + 1) % size == tail
 * - 空条件：head == tail
 */

#include "psram_ring.h"
#include <Arduino.h>
#include <esp_heap_caps.h>
#include <atomic>

// 环形缓冲区结构
static uint8_t* g_ring_buffer = nullptr;
static uint32_t g_ring_size = 0;
static std::atomic<uint32_t> g_ring_head{0};  // 写入位置（生产者）
static std::atomic<uint32_t> g_ring_tail{0};  // 读取位置（消费者）
static bool g_ring_ready = false;

// 初始化环形缓冲区
void psram_ring_init(uint32_t size_bytes) {
    if (g_ring_ready) {
        Serial.println("⚠️ PSRAM ring已初始化，跳过");
        return;
    }
    
    // 尝试从PSRAM分配内存
    g_ring_buffer = (uint8_t*)heap_caps_malloc(size_bytes, MALLOC_CAP_SPIRAM);
    
    if (!g_ring_buffer) {
        Serial.printf("❌ PSRAM ring分配失败: %lu bytes\n", size_bytes);
        g_ring_ready = false;
        return;
    }
    
    g_ring_size = size_bytes;
    g_ring_head.store(0, std::memory_order_release);
    g_ring_tail.store(0, std::memory_order_release);
    g_ring_ready = true;
    
    Serial.printf("✅ PSRAM ring初始化成功: %lu bytes (%.2f MB)\n", 
                  size_bytes, size_bytes / 1024.0 / 1024.0);
}

// 释放环形缓冲区
void psram_ring_deinit(void) {
    if (g_ring_buffer) {
        heap_caps_free(g_ring_buffer);
        g_ring_buffer = nullptr;
    }
    g_ring_size = 0;
    g_ring_head.store(0, std::memory_order_release);
    g_ring_tail.store(0, std::memory_order_release);
    g_ring_ready = false;
}

void psram_ring_reset(void) {
    if (!g_ring_ready) {
        return;
    }

    uint32_t head = g_ring_head.load(std::memory_order_acquire);
    g_ring_tail.store(head, std::memory_order_release);
}

// 生产者写入数据
uint32_t psram_ring_write(const uint8_t* data, uint32_t len) {
    if (!g_ring_ready || !data || len == 0) {
        return 0;
    }
    
    // 读取当前head和tail（使用memory_order_acquire保证可见性）
    uint32_t head = g_ring_head.load(std::memory_order_acquire);
    uint32_t tail = g_ring_tail.load(std::memory_order_acquire);
    
    // 计算可用空间（留一个字节作为满/空判断）
    uint32_t available;
    if (head >= tail) {
        available = g_ring_size - (head - tail) - 1;
    } else {
        available = tail - head - 1;
    }
    
    // 空间不足，返回0
    if (available < len) {
        return 0;
    }
    
    // 写入数据（可能需要分两段）
    uint32_t written = 0;
    
    if (head + len <= g_ring_size) {
        // 一次性写入
        memcpy(g_ring_buffer + head, data, len);
        written = len;
        head = (head + len) % g_ring_size;
    } else {
        // 分两段写入
        uint32_t first_part = g_ring_size - head;
        memcpy(g_ring_buffer + head, data, first_part);
        memcpy(g_ring_buffer, data + first_part, len - first_part);
        written = len;
        head = len - first_part;
    }
    
    // 更新head（使用memory_order_release保证写入完成后才更新）
    g_ring_head.store(head, std::memory_order_release);
    
    return written;
}

// 消费者读取数据
uint32_t psram_ring_read(uint8_t* buffer, uint32_t max_len) {
    if (!g_ring_ready || !buffer || max_len == 0) {
        return 0;
    }
    
    // 读取当前head和tail
    uint32_t head = g_ring_head.load(std::memory_order_acquire);
    uint32_t tail = g_ring_tail.load(std::memory_order_acquire);
    
    // 计算可读数据量
    uint32_t available;
    if (head >= tail) {
        available = head - tail;
    } else {
        available = g_ring_size - tail + head;
    }
    
    // 没有数据，返回0
    if (available == 0) {
        return 0;
    }
    
    // 读取数据（最多max_len字节）
    uint32_t to_read = (available < max_len) ? available : max_len;
    uint32_t read_count = 0;
    
    if (tail + to_read <= g_ring_size) {
        // 一次性读取
        memcpy(buffer, g_ring_buffer + tail, to_read);
        read_count = to_read;
        tail = (tail + to_read) % g_ring_size;
    } else {
        // 分两段读取
        uint32_t first_part = g_ring_size - tail;
        memcpy(buffer, g_ring_buffer + tail, first_part);
        memcpy(buffer + first_part, g_ring_buffer, to_read - first_part);
        read_count = to_read;
        tail = to_read - first_part;
    }
    
    // 更新tail
    g_ring_tail.store(tail, std::memory_order_release);
    
    return read_count;
}

// 当前可读字节数
uint32_t psram_ring_available_read(void) {
    if (!g_ring_ready) {
        return 0;
    }
    
    uint32_t head = g_ring_head.load(std::memory_order_acquire);
    uint32_t tail = g_ring_tail.load(std::memory_order_acquire);
    
    if (head >= tail) {
        return head - tail;
    } else {
        return g_ring_size - tail + head;
    }
}

// 当前可写字节数
uint32_t psram_ring_available_write(void) {
    if (!g_ring_ready) {
        return 0;
    }
    
    uint32_t head = g_ring_head.load(std::memory_order_acquire);
    uint32_t tail = g_ring_tail.load(std::memory_order_acquire);
    
    uint32_t available;
    if (head >= tail) {
        available = g_ring_size - (head - tail) - 1;
    } else {
        available = tail - head - 1;
    }
    
    return available;
}

// 是否已初始化
bool psram_ring_is_ready(void) {
    return g_ring_ready;
}
