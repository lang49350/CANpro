/*
 * UCAN-FD command handler.
 * Note: file name kept for compatibility with existing includes.
 */

#ifndef UCAN_HANDLER_H
#define UCAN_HANDLER_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "../Config/config.h"

class CANManager;

class UCANHandler {
public:
    UCANHandler();

    bool init(QueueHandle_t sendQueue, QueueHandle_t freeQueue,
              QueueHandle_t usbToCanQueue1 = nullptr,
              QueueHandle_t usbToCanQueue2 = nullptr);

    void setCANManager(CANManager* canManager);
    void processCommand(uint8_t type, uint8_t flags, uint8_t seq,
                        const uint8_t* data = nullptr, uint16_t len = 0);

    QueueHandle_t getHighPriorityQueue() { return m_highPriorityQueue; }

private:
    struct ChannelConfig {
        bool enabled = true;
        uint8_t modeBits = UCAN_CH_MODE_EN | UCAN_CH_MODE_RX_EN | UCAN_CH_MODE_TX_EN;
        uint8_t protoBits = 0;
        uint32_t nominalBps = 500000;
        uint32_t dataBps = 2000000;
    };

    QueueHandle_t m_sendQueue;
    QueueHandle_t m_freeQueue;
    QueueHandle_t m_usbToCanQueue1;
    QueueHandle_t m_usbToCanQueue2;
    QueueHandle_t m_highPriorityQueue;
    CANManager* m_canManager;
    ChannelConfig m_channelCfg[2];

    bool sendPacket(uint8_t type, uint8_t flags, uint8_t seq,
                    const uint8_t* payload, uint16_t payloadLen,
                    bool highPriority = false);
    bool sendAck(uint8_t reqType, uint8_t reqSeq, uint8_t status, uint8_t detail = 0);

    void handleHello(uint8_t flags, uint8_t seq);
    void handleChannelSet(uint8_t flags, uint8_t seq, const uint8_t* data, uint16_t len);
    void handleChannelGet(uint8_t flags, uint8_t seq, const uint8_t* data, uint16_t len);
    void handleStreamSet(uint8_t flags, uint8_t seq, const uint8_t* data, uint16_t len);
    void handleTxBatch(uint8_t flags, uint8_t seq, const uint8_t* data, uint16_t len);
    void handleStatsGet(uint8_t flags, uint8_t seq);

    static uint8_t dlcToBytes(uint8_t dlc);
};

#endif // UCAN_HANDLER_H
