# Requirements Document

## Introduction

本文档定义ESP32-S3 ZCAN Gateway的USB CDC接收优化需求，旨在解决当前系统中频繁出现的"CDC RX Overflow"错误。当前系统在高速数据流场景下，上位机发送数据的速度超过ESP32的处理能力，导致每14ms出现一次溢出错误。本需求将通过优化USB接收处理机制、增加缓冲容量、实现流控机制来彻底解决此问题。

## Glossary

- **USB_CDC_Handler**: USB CDC通信处理模块，负责接收和处理来自上位机的USB数据
- **Protocol_Manager**: 协议管理器，负责解析和处理接收到的协议数据
- **RX_Buffer**: 接收缓冲区，用于临时存储从USB CDC接收的数据
- **Flow_Control**: 流控机制，用于在缓冲区接近满时通知上位机减速或暂停发送
- **Overflow_Error**: 溢出错误，当接收数据速度超过处理速度导致缓冲区满时发生
- **Upper_Computer**: 上位机，通过USB CDC与ESP32-S3通信的PC端应用程序
- **CAN_Frame**: CAN总线数据帧，网关需要转发的核心数据单元
- **USBCDC_Class**: Arduino框架提供的USB CDC通信类
- **FreeRTOS_Task**: FreeRTOS操作系统中的任务单元

## Requirements

### Requirement 1: 高速USB数据接收

**User Story:** 作为系统开发者，我希望USB CDC接收任务能够高速处理数据，以便消除接收溢出错误。

#### Acceptance Criteria

1. WHEN USB CDC有可用数据时，THE USB_CDC_Handler SHALL一次读取所有可用数据而不是固定的64字节
2. WHEN处理接收数据时，THE USB_CDC_Handler SHALL在单次循环中处理多个数据包而不引入固定延迟
3. WHEN RX_Buffer有足够空间时，THE USB_CDC_Handler SHALL持续读取直到USB缓冲区为空或RX_Buffer接近满
4. THE USB_CDC_Handler SHALL在10ms内处理至少1KB的接收数据

### Requirement 2: 扩展接收缓冲区

**User Story:** 作为系统开发者，我希望增加接收缓冲区容量，以便在数据突发时有足够的缓冲空间。

#### Acceptance Criteria

1. THE RX_Buffer SHALL提供至少4KB的存储容量
2. WHEN RX_Buffer被创建时，THE System SHALL成功分配所需内存
3. WHEN RX_Buffer达到80%容量时，THE System SHALL触发流控机制
4. THE RX_Buffer SHALL支持线程安全的读写操作

### Requirement 3: 流控机制

**User Story:** 作为系统开发者，我希望实现流控机制，以便在处理能力不足时通知上位机减速。

#### Acceptance Criteria

1. WHEN RX_Buffer使用率超过80%时，THE Flow_Control SHALL向Upper_Computer发送暂停信号
2. WHEN RX_Buffer使用率降至50%以下时，THE Flow_Control SHALL向Upper_Computer发送恢复信号
3. THE Flow_Control SHALL使用标准的USB CDC流控协议（如XON/XOFF或硬件流控）
4. WHEN流控信号发送失败时，THE System SHALL记录错误并继续尝试处理数据

### Requirement 4: 优化数据处理流程

**User Story:** 作为系统开发者，我希望优化数据从USB到Protocol_Manager的传递流程，以便减少处理延迟。

#### Acceptance Criteria

1. WHEN数据从USB CDC读取后，THE USB_CDC_Handler SHALL直接将数据传递给Protocol_Manager而不进行不必要的复制
2. THE USB_CDC_Handler SHALL使用零拷贝或最小拷贝策略传递数据
3. WHEN Protocol_Manager处理数据时，THE System SHALL允许USB_CDC_Handler继续接收新数据
4. THE System SHALL使用FreeRTOS队列或环形缓冲区实现生产者-消费者模式

### Requirement 5: 错误监控和恢复

**User Story:** 作为系统维护者，我希望系统能够监控溢出错误并自动恢复，以便提高系统可靠性。

#### Acceptance Criteria

1. WHEN Overflow_Error发生时，THE System SHALL记录错误事件和时间戳
2. WHEN连续发生3次Overflow_Error时，THE System SHALL触发流控机制
3. WHEN Overflow_Error发生时，THE System SHALL清理损坏的数据包并继续处理后续数据
4. THE System SHALL提供溢出错误计数器供诊断使用
5. WHEN系统运行稳定超过60秒无溢出时，THE System SHALL重置错误计数器

### Requirement 6: 性能指标

**User Story:** 作为系统开发者，我希望系统达到明确的性能指标，以便验证优化效果。

#### Acceptance Criteria

1. THE System SHALL在持续高速数据流（1MB/s）下运行至少5分钟无Overflow_Error
2. THE USB_CDC_Handler SHALL保持CPU使用率低于30%
3. THE System SHALL在接收到数据后100ms内完成处理并传递给Protocol_Manager
4. WHEN测试环境模拟上位机发送大量CAN帧配置时，THE System SHALL成功接收并处理所有数据包

### Requirement 7: 配置和调试支持

**User Story:** 作为系统开发者，我希望能够配置和调试USB接收参数，以便在不同场景下优化性能。

#### Acceptance Criteria

1. THE System SHALL提供编译时配置选项用于调整RX_Buffer大小
2. THE System SHALL提供编译时配置选项用于启用/禁用流控机制
3. WHEN调试模式启用时，THE System SHALL输出USB接收统计信息（接收速率、缓冲区使用率、溢出次数）
4. THE System SHALL提供运行时命令用于查询当前缓冲区状态

### Requirement 8: 向后兼容性

**User Story:** 作为系统集成者，我希望优化后的系统保持与现有Protocol_Manager的兼容性，以便无需修改上层协议处理逻辑。

#### Acceptance Criteria

1. THE USB_CDC_Handler SHALL保持与现有Protocol_Manager接口的兼容性
2. WHEN Protocol_Manager调用现有API时，THE System SHALL正常工作
3. THE System SHALL保持现有的数据格式和协议不变
4. WHEN系统升级后，THE System SHALL能够与未升级的上位机软件正常通信
