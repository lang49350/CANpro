/*
 * UCAN-FD v1 protocol constants.
 * Single supported protocol for both firmware and host.
 */

#ifndef PROTOCOL_CONFIG_H
#define PROTOCOL_CONFIG_H

// ============================================================================
// Packet envelope
// ============================================================================
#define UCAN_MAGIC                  0xA6
#define UCAN_HEADER_SIZE            6
#define UCAN_BATCH_HEADER_SIZE      10

// Header flags
#define UCAN_FLAG_ACK_REQ           0x01
#define UCAN_FLAG_IS_RESP           0x02
#define UCAN_FLAG_ERR               0x04

// ============================================================================
// Message types
// ============================================================================
#define UCAN_MSG_HELLO_REQ          0x01
#define UCAN_MSG_HELLO_RSP          0x02
#define UCAN_MSG_ACK                0x03
#define UCAN_MSG_EVENT              0x04

#define UCAN_MSG_CH_SET             0x10
#define UCAN_MSG_CH_GET             0x11
#define UCAN_MSG_CH_STATE           0x12
#define UCAN_MSG_STREAM_SET         0x14

#define UCAN_MSG_TX_BATCH           0x20
#define UCAN_MSG_RX_BATCH           0x21

#define UCAN_MSG_STATS_GET          0x30
#define UCAN_MSG_STATS_RSP          0x31

// ============================================================================
// Protocol/version/capabilities
// ============================================================================
#define UCAN_VERSION_MAJOR          1
#define UCAN_VERSION_MINOR          0

#define UCAN_CAP_CAN20              0x00000001UL
#define UCAN_CAP_CANFD              0x00000002UL
#define UCAN_CAP_BATCH              0x00000004UL
#define UCAN_CAP_DUAL_CHANNEL       0x00000008UL

// ============================================================================
// Status codes
// ============================================================================
#define UCAN_STATUS_OK              0x00
#define UCAN_STATUS_BAD_LEN         0x01
#define UCAN_STATUS_BAD_PARAM       0x02
#define UCAN_STATUS_UNSUPPORTED     0x03
#define UCAN_STATUS_QUEUE_FULL      0x04
#define UCAN_STATUS_INTERNAL_ERR    0x05

// ============================================================================
// Channel / frame bits
// ============================================================================
#define UCAN_CH_MODE_EN             0x01
#define UCAN_CH_MODE_RX_EN          0x02
#define UCAN_CH_MODE_TX_EN          0x04
#define UCAN_CH_MODE_LISTEN_ONLY    0x08
#define UCAN_CH_MODE_LOOPBACK       0x10
#define UCAN_CH_MODE_ONE_SHOT       0x20

#define UCAN_PROTO_CANFD_EN         0x01
#define UCAN_PROTO_BRS_ALLOW        0x02

#define UCAN_ID_EXT_BIT             0x80000000UL
#define UCAN_ID_FD_BIT              0x40000000UL
#define UCAN_ID_BRS_BIT             0x20000000UL
#define UCAN_ID_ESI_BIT             0x10000000UL
#define UCAN_ID_RTR_BIT             0x08000000UL

// Batch flags
#define UCAN_BATCH_FLAG_HAS_RECORD_BYTES 0x01

// ============================================================================
// Buffer / structures
// ============================================================================
#define UCAN_PACKET_MAX_SIZE        1024
#define UCAN_MAX_PAYLOAD            (UCAN_PACKET_MAX_SIZE - UCAN_HEADER_SIZE)

typedef struct {
    uint8_t data[UCAN_PACKET_MAX_SIZE];
    uint16_t length;
} UCANPacket;

typedef struct {
    uint32_t idFlags;
    uint8_t meta;
    uint16_t dtUs;
    uint8_t data[64];
} __attribute__((packed)) UCANFrameData;

#endif // PROTOCOL_CONFIG_H
