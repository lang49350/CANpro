/*
 * ESP32-S3 ZCAN Gateway - PSRAM 环形缓冲区
 * 单生产者单消费者，原子 head/tail，用于 CAN→USB 不丢包路径
 */

#ifndef PSRAM_RING_H
#define PSRAM_RING_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 环形缓冲：由主程序在 setup 中初始化
void psram_ring_init(uint32_t size_bytes);
void psram_ring_deinit(void);
void psram_ring_reset(void);

// 生产者（CAN RX 任务）写入。返回写入字节数，0 表示空间不足。
// 不在 ISR 中调用，在任务中调用。
uint32_t psram_ring_write(const uint8_t* data, uint32_t len);

// 消费者（USB TX 任务）读取。返回实际读取字节数。
uint32_t psram_ring_read(uint8_t* buffer, uint32_t max_len);

// 当前可读字节数、可写字节数（近似，用于统计）
uint32_t psram_ring_available_read(void);
uint32_t psram_ring_available_write(void);

// 是否已初始化
bool psram_ring_is_ready(void);

#ifdef __cplusplus
}
#endif

#endif /* PSRAM_RING_H */
