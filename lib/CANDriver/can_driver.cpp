/*
 * ESP32-S3 ZCAN Gateway - CAN驱动实现
 */

#include "can_driver.h"
#include "can_interrupt.h"

using CANBusType::BUS_UNKNOWN;
using CANBusType::BUS_CANFD_ONLY;

CANDriver* CANDriver::instances[2] = {nullptr, nullptr};

static void mcp2518fdSoftReset(SPIClass* spi, uint8_t csPin, uint32_t spiHz)
{
    if (!spi) {
        return;
    }

    spi->beginTransaction(SPISettings(spiHz, MSBFIRST, SPI_MODE0));
    digitalWrite(csPin, LOW);
    spi->transfer((uint8_t)0x00);
    digitalWrite(csPin, HIGH);
    spi->endTransaction();

    delay(5);
}

void IRAM_ATTR CANDriver::can1ISR_Wrapper() {
    if (instances[0]) {
        // 1. 处理硬件中断（清除中断标志，读取状态）
        instances[0]->m_can->isr();
        
        // 2. 更新统计
        instances[0]->handleInterrupt();
        
        // 3. 通知上层任务
        if (instances[0]->m_notificationCallback) {
            instances[0]->m_notificationCallback();
        }
    }
}

void IRAM_ATTR CANDriver::can2ISR_Wrapper() {
    if (instances[1]) {
        instances[1]->m_can->isr();
        instances[1]->handleInterrupt();
        if (instances[1]->m_notificationCallback) {
            instances[1]->m_notificationCallback();
        }
    }
}

CANDriver::CANDriver(uint8_t channelId, ACAN2517FD* can, SPIClass* spi,
                     uint8_t csPin, uint8_t intPin)
    : m_channelId(channelId)
    , m_can(can)
    , m_spi(spi)
    , m_csPin(csPin)
    , m_intPin(intPin)
{
    memset(&m_stats, 0, sizeof(m_stats));
    m_stats.busType = BUS_UNKNOWN;
    instances[channelId] = this;
}

bool CANDriver::init(bool autoDetect) {
    Serial0.printf("[CAN%d] 开始初始化SPI和引脚...\n", m_channelId + 1);
    
    // 配置中断引脚
    pinMode(m_intPin, INPUT_PULLUP);
    Serial0.printf("[CAN%d] INT引脚配置完成: GPIO%d\n", m_channelId + 1, m_intPin);
    
    delay(50);
    
    // 初始化SPI
    if (m_channelId == 0) {
        m_spi->begin(CAN1_SCLK, CAN1_MISO, CAN1_MOSI, m_csPin);
    } else {
        m_spi->begin(CAN2_SCLK, CAN2_MISO, CAN2_MOSI, m_csPin);
    }
    
    delay(50);
    
    // 软复位 MCP2518FD，但不启动 CAN 通信
    m_spi->setFrequency(20000000);
    mcp2518fdSoftReset(m_spi, m_csPin, 20000000);
    
    // 我们不再这里调用 tryInitialize() 启动 CAN，而是等待上位机下发配置后调用 start()
    Serial0.printf("[CAN%d] ✅ SPI和硬件复位成功，等待配置启动...\n", m_channelId + 1);
    
    return true;
}

bool CANDriver::start(uint32_t bitrate, uint32_t fdBitrate, bool isFD, WorkMode mode) {
    Serial0.printf("[CAN%d] 启动CAN控制器 (波特率=%lu, FD波特率=%lu, 模式=%d, 工作模式=%d)...\n", 
                   m_channelId + 1, bitrate, fdBitrate, isFD, mode);
    
    const uint32_t SPI_FREQ = 20000000;
    const ACAN2517FDSettings::Oscillator OSC_FREQ = ACAN2517FDSettings::OSC_40MHz;
    
    m_spi->setFrequency(SPI_FREQ);
    mcp2518fdSoftReset(m_spi, m_csPin, SPI_FREQ);
    
    // 这里使用简化的倍率计算，实际可能需要更精确的配置
    DataBitRateFactor factor = DataBitRateFactor::x1;
    if (isFD) {
        if (fdBitrate >= 4000000) factor = DataBitRateFactor::x8;
        else if (fdBitrate >= 2000000) factor = DataBitRateFactor::x4;
        else if (fdBitrate >= 1000000) factor = DataBitRateFactor::x2;
    }
    
    ACAN2517FDSettings settings(OSC_FREQ, bitrate, factor);
    settings.mRequestedMode = isFD ? ACAN2517FDSettings::NormalFD : ACAN2517FDSettings::Normal20B;

    // Classic CAN 必须用 PAYLOAD_8 计控制器 RAM；默认 PAYLOAD_64 时 31+1 槽 = 32×76=2432 > 2048，begin 返回 0x1000
    if (!isFD) {
        settings.mControllerReceiveFIFOPayload = ACAN2517FDSettings::PAYLOAD_8;
        settings.mControllerTransmitFIFOPayload = ACAN2517FDSettings::PAYLOAD_8;
    }

    // 根据工作模式分配 FIFO (MCP2518FD 只有 2048 字节 RAM)
    // 🚀 极致性能优化：逼近硬件极限
    // CAN FD (64B Payload): 每个对象 76B => 最大 26 个 (保守设 25)
    // CAN 2.0 (8B Payload): 每个对象 20B => 最大 32 个 (硬件限制 32)
    
    uint8_t maxFifoFD = 24;  // 24 + 1 = 25 < 26 (留余量)
    uint8_t maxFifo20 = 31;  // 硬件极限
    
    uint8_t rxSize, txSize;
    
    if (mode == WORK_MODE_RX_ONLY) {
        rxSize = isFD ? maxFifoFD : maxFifo20;
        txSize = 1;
    } else if (mode == WORK_MODE_TX_ONLY) {
        // 并线时本口仍会收到总线上全部报文，硬件 RX 不能为 1，否则 ISR 来不及搬运会大量丢帧/异常
        rxSize = isFD ? 2u : 15u;
        txSize = isFD ? maxFifoFD : maxFifo20;
    } else { // WORK_MODE_NORMAL
        // 混合模式下，平分资源
        // FD: 25/2 = 12
        // 2.0: 30/2 = 15
        uint8_t halfFifo = isFD ? 12 : 15;
        rxSize = halfFifo;
        txSize = halfFifo;
    }
    
    settings.mControllerReceiveFIFOSize = rxSize;
    settings.mControllerTransmitFIFOSize = txSize;
    
    // 库内接收队列：回放/满载Classic时 64 极易溢出，帧未到 receive() 即被丢弃，表现为并线 RX≪对端 TX
    settings.mDriverReceiveFIFOSize = 512;
    settings.mDriverTransmitFIFOSize = 256;
    
    // 🔍 调试 RAM 占用
    uint32_t ramUsage = settings.ramUsage();
    Serial0.printf("[CAN%d] RAM Usage: %lu bytes (Max 2048)\n", m_channelId + 1, ramUsage);
    Serial0.printf("[CAN%d] Config: RX=%d, TX=%d, TXQ=%d, Payload=%d\n", 
                   m_channelId + 1, 
                   settings.mControllerReceiveFIFOSize,
                   settings.mControllerTransmitFIFOSize,
                   settings.mControllerTXQSize,
                   (int)settings.mControllerReceiveFIFOPayload);
    
    void (*isr)() = (m_channelId == 0) ? can1ISR_Wrapper : can2ISR_Wrapper;
    
    uint32_t errorCode = m_can->begin(settings, isr);
    
    if (errorCode == 0) {
        Serial0.printf("[CAN%d] ✅ CAN启动成功\n", m_channelId + 1);
        enableInterrupt();
        return true;
    } else {
        Serial0.printf("[CAN%d] ❌ CAN启动失败，错误码: 0x%08X\n", m_channelId + 1, errorCode);
        return false;
    }
}

void CANDriver::stop() {
    Serial0.printf("[CAN%d] 停止CAN控制器...\n", m_channelId + 1);
    disableInterrupt();
    // 软复位将使芯片进入 Configuration 模式，从而停止所有收发
    mcp2518fdSoftReset(m_spi, m_csPin, 20000000);
}

bool CANDriver::tryInitialize() {
    // 简化初始化：只使用 40MHz 晶振配置
    // MCP2518FD 推荐使用 40MHz 晶振以获得最佳性能
    // SPI 频率固定为 20MHz (MCP2518FD上限)
    
    const uint32_t SPI_FREQ = 20000000;
    const ACAN2517FDSettings::Oscillator OSC_FREQ = ACAN2517FDSettings::OSC_40MHz;
    const char* OSC_NAME = "40MHz";
    
    m_spi->setFrequency(SPI_FREQ);
    
    // 软复位
    mcp2518fdSoftReset(m_spi, m_csPin, SPI_FREQ);
    
    // CAN FD 数据域使用 x4 倍率 (2Mbps)，与仲裁域 (500kbps) 配合
    ACAN2517FDSettings settings(
        OSC_FREQ,
        CAN_DEFAULT_BITRATE,
        DataBitRateFactor::x4  // 2Mbps数据速率
    );
    
    // 启用 CAN FD 模式
    settings.mRequestedMode = ACAN2517FDSettings::NormalFD;  // CAN FD模式
    settings.mDriverReceiveFIFOSize = 32;
    settings.mDriverTransmitFIFOSize = 16;
    
    // 🔧 Fix: Ensure the controller RAM usage is within 2048 bytes
    // Since we enabled RXTSEN, each object is 4 bytes larger
    // Max payload is 64 bytes (PAYLOAD_64), so 72 + 4 = 76 bytes per object
    // 76 * mControllerReceiveFIFOSize must be <= 2048 (along with TX FIFO)
    settings.mControllerReceiveFIFOSize = 16; // reduced from 27 to 16
    settings.mControllerTransmitFIFOSize = 1;
    
    // 注册ISR
    void (*isr)() = (m_channelId == 0) ? 
        can1ISR_Wrapper : 
        can2ISR_Wrapper;
    
    uint32_t errorCode = m_can->begin(settings, isr);
    
    if (errorCode == 0) {
        Serial0.printf("[CAN%d] ✅ 配置成功: SPI=%luHz, 晶振=%s, 模式=CAN FD\n",
                    m_channelId + 1, SPI_FREQ, OSC_NAME);
        Serial0.printf("[CAN%d] 🚀 FIFO中断已由库配置: 非空+半满+溢出中断\n",
                    m_channelId + 1);
        return true;
    }
    
    Serial0.printf("[CAN%d] ❌ 初始化失败，错误码: 0x%08X\n", m_channelId + 1, errorCode);
    
    // 如果失败，可能是晶振不匹配，尝试 20MHz 备用方案
    Serial0.printf("[CAN%d] 尝试备用配置 (20MHz晶振)...\n", m_channelId + 1);
    
    mcp2518fdSoftReset(m_spi, m_csPin, SPI_FREQ);
    ACAN2517FDSettings settings20(
        ACAN2517FDSettings::OSC_20MHz,
        CAN_DEFAULT_BITRATE,
        ACAN2517FDSettings::DATA_BITRATE_x4
    );
    settings20.mRequestedMode = ACAN2517FDSettings::NormalFD;
    settings20.mDriverReceiveFIFOSize = CAN_FIFO_SIZE;
    settings20.mDriverTransmitFIFOSize = 16;
    
    errorCode = m_can->begin(settings20, isr);
    if (errorCode == 0) {
        Serial0.printf("[CAN%d] ✅ 备用配置成功: 20MHz晶振\n", m_channelId + 1);
        return true;
    }
    
    return false;
}

uint32_t CANDriver::readRegister32(uint16_t address) {
    uint8_t buffer[7];
    buffer[0] = 0x03;  // READ命令
    buffer[1] = (address >> 8) & 0xFF;
    buffer[2] = address & 0xFF;
    
    m_spi->beginTransaction(SPISettings(SPI_FREQUENCY, MSBFIRST, SPI_MODE0));
    digitalWrite(m_csPin, LOW);
    m_spi->transfer(buffer, 7);
    digitalWrite(m_csPin, HIGH);
    m_spi->endTransaction();
    
    return (static_cast<uint32_t>(buffer[3]) << 24) |
           (static_cast<uint32_t>(buffer[4]) << 16) |
           (static_cast<uint32_t>(buffer[5]) << 8) |
           static_cast<uint32_t>(buffer[6]);
}

void CANDriver::writeRegister32(uint16_t address, uint32_t value) {
    uint8_t buffer[7];
    buffer[0] = 0x02;  // WRITE命令
    buffer[1] = (address >> 8) & 0xFF;
    buffer[2] = address & 0xFF;
    buffer[3] = (value >> 24) & 0xFF;
    buffer[4] = (value >> 16) & 0xFF;
    buffer[5] = (value >> 8) & 0xFF;
    buffer[6] = value & 0xFF;
    
    m_spi->beginTransaction(SPISettings(SPI_FREQUENCY, MSBFIRST, SPI_MODE0));
    digitalWrite(m_csPin, LOW);
    m_spi->transfer(buffer, 7);
    digitalWrite(m_csPin, HIGH);
    m_spi->endTransaction();
}

void CANDriver::printRegisters() {
    // 读取关键状态寄存器
    // C1INT: 0x01C
    // C1FIFOSTA1: 0x050
    uint32_t c1int = readRegister32(0x01C);
    uint32_t fifo1sta = readRegister32(0x050);
    
    Serial0.printf("[CAN%d] DEBUG: C1INT=0x%08X, FIFO1STA=0x%08X\n", 
                 m_channelId + 1, c1int, fifo1sta);
}

void CANDriver::poll() {
    // 强制调用ISR以处理任何挂起的中断或数据
    m_can->isr();
}

void CANDriver::setNotificationCallback(void (*callback)()) {
    m_notificationCallback = callback;
}

void CANDriver::enableInterrupt() {
    void (*isr)() = (m_channelId == 0) ? can1ISR_Wrapper : can2ISR_Wrapper;
    ::attachInterrupt(digitalPinToInterrupt(m_intPin), isr, FALLING);  // 使用下降沿触发
}

void CANDriver::disableInterrupt() {
    ::detachInterrupt(digitalPinToInterrupt(m_intPin));
}

void CANDriver::handleInterrupt() {
    m_stats.interruptCount++;
}

uint8_t CANDriver::receiveBatch(CANFDMessage* msgs, uint8_t maxCount) {
    uint8_t count = 0;
    
    while (count < maxCount && m_can->available()) {
        if (m_can->receive(msgs[count])) {
            count++;
            m_stats.rxCount++;
        } else {
            break;
        }
    }
    
    return count;
}

bool CANDriver::send(const CANFDMessage& msg) {
    if (m_can->tryToSend(msg)) {
        m_stats.txCount++;
        return true;
    }
    
    m_stats.errorCount++;
    return false;
}

bool CANDriver::sendBatch(const CANFDMessage* msgs, uint8_t count) {
    uint8_t sent = 0;
    
    for (uint8_t i = 0; i < count; i++) {
        if (send(msgs[i])) {
            sent++;
        }
    }
    
    return sent == count;
}

bool CANDriver::detectBusType() {
    // 快速检测：只监听100ms
    // 如果没有数据，默认为CAN 2.0
    
    Serial0.printf("[CAN%d] 快速检测总线类型（监听100ms）...\n", m_channelId + 1);
    
    uint32_t can20Count = 0;
    uint32_t canfdCount = 0;
    uint64_t startTime = millis();
    CANFDMessage msgs[10];
    
    while (millis() - startTime < 100) {  // 监听100ms
        // 使用轮询模式检测
        m_can->isr();
        uint8_t count = receiveBatch(msgs, 10);
        
        for (uint8_t i = 0; i < count; i++) {
            if (msgs[i].len > 8 || 
                msgs[i].type == CANFDMessage::CANFD_NO_BIT_RATE_SWITCH ||
                msgs[i].type == CANFDMessage::CANFD_WITH_BIT_RATE_SWITCH) {
                canfdCount++;
            } else {
                can20Count++;
            }
        }
        
        if (can20Count + canfdCount >= 10) { // 收到10帧就足够了
            break;
        }
        
        delay(10);
    }
    
    // 默认判断为CAN 2.0，除非明确收到FD帧
    if (canfdCount > 0) {
        m_stats.busType = (can20Count > 0) ? CANBusType::BUS_MIXED : CANBusType::BUS_CANFD_ONLY;
    } else {
        m_stats.busType = CANBusType::BUS_CAN20_ONLY;
    }
    
    Serial0.printf("[CAN%d] 检测完成: %s (CAN2.0: %lu, CANFD: %lu)\n", 
                 m_channelId + 1, 
                 canfdCount > 0 ? "CAN FD" : "CAN 2.0",
                 can20Count, canfdCount);
    
    return true; // 总是返回成功，不阻塞启动
}

bool CANDriver::setBitrate(uint32_t bitrate) {
    // 动态修改波特率
    // 注意：需要重新初始化MCP2518FD
    
    Serial0.printf("[CAN%d] 设置波特率: %lu bps\n", m_channelId + 1, bitrate);
    
    // 创建新的配置
    ACAN2517FDSettings settings(
        ACAN2517FDSettings::OSC_40MHz,
        bitrate,
        ACAN2517FDSettings::DATA_BITRATE_x4
    );
    
    settings.mRequestedMode = ACAN2517FDSettings::NormalFD;
    settings.mDriverReceiveFIFOSize = CAN_FIFO_SIZE;
    settings.mDriverTransmitFIFOSize = 16;  // 🔧 修复：增加发送缓冲区
    
    // 临时ISR
    void (*isr)() = (m_channelId == 0) ? 
        can1ISR_Wrapper : 
        can2ISR_Wrapper;
    
    // 重新初始化
    uint32_t errorCode = m_can->begin(settings, isr);
    
    if (errorCode == 0) {
        Serial0.printf("[CAN%d] ✅ 波特率设置成功\n", m_channelId + 1);
        return true;
    } else {
        Serial0.printf("[CAN%d] ❌ 波特率设置失败，错误码: 0x%08X\n", m_channelId + 1, errorCode);
        return false;
    }
}

bool CANDriver::setFDBitrate(uint32_t dataBitrate) {
    // 动态修改CAN FD数据速率
    
    Serial0.printf("[CAN%d] 设置CAN FD数据速率: %lu bps\n", m_channelId + 1, dataBitrate);
    
    // 简化实现：直接使用固定倍率
    // 创建新的配置，使用4倍数据速率（最常用）
    ACAN2517FDSettings settings(
        ACAN2517FDSettings::OSC_40MHz,
        CAN_DEFAULT_BITRATE,
        ACAN2517FDSettings::DATA_BITRATE_x4
    );
    
    settings.mRequestedMode = ACAN2517FDSettings::NormalFD;
    settings.mDriverReceiveFIFOSize = CAN_FIFO_SIZE;
    settings.mDriverTransmitFIFOSize = 16;  // 🔧 修复：增加发送缓冲区
    
    // 临时ISR
    void (*isr)() = (m_channelId == 0) ? 
        can1ISR_Wrapper : 
        can2ISR_Wrapper;
    
    // 重新初始化
    uint32_t errorCode = m_can->begin(settings, isr);
    
    if (errorCode == 0) {
        Serial0.printf("[CAN%d] ✅ CAN FD数据速率设置成功\n", m_channelId + 1);
        return true;
    } else {
        Serial0.printf("[CAN%d] ❌ CAN FD数据速率设置失败，错误码: 0x%08X\n", m_channelId + 1, errorCode);
        return false;
    }
}

bool CANDriver::setMode(bool canFD) {
    // 动态切换CAN/CAN FD模式
    
    Serial0.printf("[CAN%d] 切换模式: %s\n", m_channelId + 1, canFD ? "CAN FD" : "CAN 2.0");
    
    // 创建新的配置
    ACAN2517FDSettings settings(
        m_oscillator, // 使用保存的晶振配置
        CAN_DEFAULT_BITRATE,
        canFD ? ACAN2517FDSettings::DATA_BITRATE_x4 : ACAN2517FDSettings::DATA_BITRATE_x1
    );
    
    // 设置模式
    if (canFD) {
        settings.mRequestedMode = ACAN2517FDSettings::NormalFD;
    } else {
        settings.mRequestedMode = ACAN2517FDSettings::Normal20B;
    }
    
    settings.mDriverReceiveFIFOSize = 31;
    settings.mDriverTransmitFIFOSize = 16;  // 🔧 修复：增加发送缓冲区
    
    // 使用静态 ISR 包装器
    void (*isr)() = (m_channelId == 0) ? can1ISR_Wrapper : can2ISR_Wrapper;
    
    // 重新初始化
    uint32_t errorCode = m_can->begin(settings, isr);
    
    if (errorCode == 0) {
        Serial0.printf("[CAN%d] ✅ 模式切换成功\n", m_channelId + 1);
        return true;
    } else {
        Serial0.printf("[CAN%d] ❌ 模式切换失败，错误码: 0x%08X\n", m_channelId + 1, errorCode);
        return false;
    }
}

void CANDriver::resetStats() {
    BusType busType = m_stats.busType;
    memset(&m_stats, 0, sizeof(m_stats));
    m_stats.busType = busType;
}
