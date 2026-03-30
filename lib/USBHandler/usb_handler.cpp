/*
 * ESP32-S3 ZCAN Gateway - USB处理器实现
 */

#include "usb_handler.h"

USBHandler::USBHandler(USBCDC* usbSerial)
    : m_usbSerial(usbSerial)
    , m_totalBytesSent(0)
    , m_totalPacketsSent(0)
{
    memset(&m_doubleBuffer, 0, sizeof(m_doubleBuffer));
}

bool USBHandler::init() {
    if (!USE_USB_DOUBLE_BUFFER) {
        return true;
    }
    
    // 创建信号量
    m_doubleBuffer.swapSemaphore = xSemaphoreCreateMutex();
    if (m_doubleBuffer.swapSemaphore == NULL) {
        Serial0.println("[USB] ❌ 信号量创建失败");
        return false;
    }
    
    m_doubleBuffer.usingA = true;
    m_doubleBuffer.sizeA = 0;
    m_doubleBuffer.sizeB = 0;
    m_doubleBuffer.sendTaskHandle = NULL;
    
    Serial0.println("[USB] ✅ 双缓冲初始化成功");
    return true;
}

void USBHandler::setSendTaskHandle(TaskHandle_t handle) {
    m_doubleBuffer.sendTaskHandle = handle;
}

uint8_t* USBHandler::getFillBuffer() {
    if (!USE_USB_DOUBLE_BUFFER) {
        return m_doubleBuffer.bufferA;
    }
    
    xSemaphoreTake(m_doubleBuffer.swapSemaphore, portMAX_DELAY);
    uint8_t* buffer = m_doubleBuffer.usingA ? 
                      m_doubleBuffer.bufferA : 
                      m_doubleBuffer.bufferB;
    xSemaphoreGive(m_doubleBuffer.swapSemaphore);
    
    return buffer;
}

uint16_t USBHandler::getFillBufferSize() {
    if (!USE_USB_DOUBLE_BUFFER) {
        return USB_BUFFER_SIZE;
    }
    
    xSemaphoreTake(m_doubleBuffer.swapSemaphore, portMAX_DELAY);
    uint16_t size = m_doubleBuffer.usingA ? 
                    m_doubleBuffer.sizeA : 
                    m_doubleBuffer.sizeB;
    xSemaphoreGive(m_doubleBuffer.swapSemaphore);
    
    return size;
}

bool USBHandler::swapBuffers(uint16_t size) {
    if (!USE_USB_DOUBLE_BUFFER) {
        return send(m_doubleBuffer.bufferA, size);
    }
    
    xSemaphoreTake(m_doubleBuffer.swapSemaphore, portMAX_DELAY);
    
    // 保存当前缓冲区大小
    if (m_doubleBuffer.usingA) {
        m_doubleBuffer.sizeA = size;
    } else {
        m_doubleBuffer.sizeB = size;
    }
    
    // 交换缓冲区
    m_doubleBuffer.usingA = !m_doubleBuffer.usingA;
    
    xSemaphoreGive(m_doubleBuffer.swapSemaphore);
    
    // 通知发送任务
    if (m_doubleBuffer.sendTaskHandle != NULL) {
        xTaskNotifyGive(m_doubleBuffer.sendTaskHandle);
    }
    
    return true;
}

bool USBHandler::sendDirect() {
    if (!USE_USB_DOUBLE_BUFFER) {
        return false;
    }
    
    // 等待通知
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    
    xSemaphoreTake(m_doubleBuffer.swapSemaphore, portMAX_DELAY);
    
    // 发送非当前缓冲区
    uint8_t* sendBuffer;
    uint16_t sendSize;
    
    if (m_doubleBuffer.usingA) {
        sendBuffer = m_doubleBuffer.bufferB;
        sendSize = m_doubleBuffer.sizeB;
    } else {
        sendBuffer = m_doubleBuffer.bufferA;
        sendSize = m_doubleBuffer.sizeA;
    }
    
    xSemaphoreGive(m_doubleBuffer.swapSemaphore);
    
    if (sendSize > 0) {
        return send(sendBuffer, sendSize);
    }
    
    return true;
}

bool USBHandler::send(const uint8_t* data, uint16_t size) {
    return sendBuffer(data, size);
}

bool USBHandler::sendBuffer(const uint8_t* buffer, uint16_t size) {
    if (size == 0) {
        return true;
    }
    
    size_t totalWritten = 0;
    uint32_t retries = 0;
    while (totalWritten < size && retries < 200) {
        size_t written = m_usbSerial->write(buffer + totalWritten, size - totalWritten);
        if (written > 0) {
            totalWritten += written;
        } else {
            retries++;
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
    
    if (totalWritten == size) {
        m_totalBytesSent += size;
        m_totalPacketsSent++;
        return true;
    }
    
    return false;
}

int USBHandler::available() {
    return m_usbSerial->available();
}

uint8_t USBHandler::read() {
    return m_usbSerial->read();
}
