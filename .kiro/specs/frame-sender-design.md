# ZCANPRO 帧发送与回放功能设计文档

## 1. 执行摘要

本文档详细设计ZCANPRO上位机的帧发送和回放功能，包括UI设计、数据流、核心逻辑和实施计划。

**设计原则**：
- 参考SavvyCAN和ZLG ZCANPRO的成熟设计
- 遵循ZCANPRO现有架构（ConnectionManager、DataRouter）
- 简化实现，先MVP后扩展
- 严格遵守500行文件限制

---

## 2. 功能概述

### 2.1 核心功能模块

```
┌─────────────────────────────────────────────────────────────┐
│                    帧发送与回放系统                          │
├─────────────────────────────────────────────────────────────┤
│  1. 简单发送 (FrameSenderWidget)                            │
│     - 周期发送                                               │
│     - 列表发送                                               │
│     - ID/数据自增                                            │
│                                                              │
│  2. 回放功能 (FramePlaybackWidget)                          │
│     - 文件回放（BLF/ASC/CSV）                               │
│     - 倍速控制                                               │
│     - 步进回放                                               │
│     - 过滤回放                                               │
│                                                              │
│  3. DBC发送 (DBCSenderWidget) - 阶段4                       │
│     - 信号编辑                                               │
│     - 自动计算                                               │
└─────────────────────────────────────────────────────────────┘
```

---

## 3. UI设计

### 3.1 简单发送窗口 (FrameSenderWidget)

**窗口布局**：
```
┌──────────────────────────────────────────────────────────────┐
│ 帧发送 - COM3 CAN0                                    [×]    │
├──────────────────────────────────────────────────────────────┤
│ 设备: [COM3 ▼]  通道: [CAN0 ▼]                              │
├──────────────────────────────────────────────────────────────┤
│ [✓] | ID    | Len | Ext | Data              | 间隔  | 次数  │
│ [✓] | 0x100 | 8   | [ ] | 00 11 22 33 ...   | 100ms | 10   │
│ [ ] | 0x200 | 8   | [✓] | AA BB CC DD ...   | 50ms  | ∞    │
│ [✓] | 0x300 | 4   | [ ] | 01 02 03 04       | 200ms | 1    │
│ [ ] | 0x400 | 8   | [ ] | FF FF FF FF ...   | 1000ms| 5    │
│                                                              │
│ [+ 添加行]                                                   │
├──────────────────────────────────────────────────────────────┤
│ 高级选项 [▼]                                                 │
│ ┌────────────────────────────────────────────────────────┐  │
│ │ [✓] ID自增   起始:0x100  步长:1  最大:0x1FF           │  │
│ │ [✓] 数据自增 字节:0  起始:0  步长:1  最大:255         │  │
│ └────────────────────────────────────────────────────────┘  │
├──────────────────────────────────────────────────────────────┤
│ [启动全部] [停止全部] [清空] [加载配置] [保存配置]          │
│                                                              │
│ 状态: 已发送 1234 帧 | 运行中                               │
└──────────────────────────────────────────────────────────────┘
```

**表格列说明**：
- **En (Enable)**: 勾选框，是否启用该行
- **ID**: 帧ID（十六进制），支持0x前缀
- **Len**: 数据长度（0-8）
- **Ext**: 扩展帧勾选框
- **Data**: 数据字节（十六进制，空格分隔）
- **间隔**: 发送间隔（ms），0表示单次发送
- **次数**: 发送次数，∞表示无限循环

### 3.2 回放窗口 (FramePlaybackWidget)

**窗口布局**：
```
┌──────────────────────────────────────────────────────────────┐
│ 帧回放                                                [×]    │
├──────────────────────────────────────────────────────────────┤
│ 设备: [COM3 ▼]  通道: [CAN0 ▼]                              │
├──────────────────────────────────────────────────────────────┤
│ 回放序列:                                                    │
│ ┌────────────────────────────────────────────────────────┐  │
│ │ 文件名                          | 循环次数               │  │
│ │ test_data.blf                   | 1                     │  │
│ │ <实时捕获数据>                  | 1                     │  │
│ │ another_test.asc                | 3                     │  │
│ └────────────────────────────────────────────────────────┘  │
│ [加载文件] [加载实时数据] [删除]                             │
├──────────────────────────────────────────────────────────────┤
│ 回放控制:                                                    │
│ [⏮ 后退] [⏸ 暂停] [◀ 倒放] [⏹ 停止] [▶ 播放] [⏭ 前进]    │
│                                                              │
│ 进度: ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ │
│       45% | 1234/2700 帧 | 用时: 00:05                      │
│                                                              │
│ 当前文件: test_data.blf                                      │
├──────────────────────────────────────────────────────────────┤
│ 回放设置:                                                    │
│ ┌────────────────────────────────────────────────────────┐  │
│ │ 模式: ⦿ 正常  ○ 步进  ○ 尽可能快                       │  │
│ │                                                          │  │
│ │ 倍速: [1.0x ▼]  步进间隔: [10ms]                        │  │
│ │                                                          │  │
│ │ [✓] 使用原始时间戳                                       │  │
│ │ [✓] 循环播放整个序列                                     │  │
│ │ [ ] 等待总线流量后开始                                   │  │
│ └────────────────────────────────────────────────────────┘  │
├──────────────────────────────────────────────────────────────┤
│ ID过滤: [全选] [全不选] [保存过滤] [加载过滤]               │
│ ┌────────────────────────────────────────────────────────┐  │
│ │ [✓] 0x100 - Message_A                                   │  │
│ │ [✓] 0x200 - Message_B                                   │  │
│ │ [ ] 0x300 - Message_C                                   │  │
│ │ [✓] 0x400 - Message_D                                   │  │
│ └────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────┘
```

**按钮说明**：
- **⏮ 后退**: 步进后退一帧
- **⏸ 暂停**: 暂停回放，可继续
- **◀ 倒放**: 倒序回放
- **⏹ 停止**: 停止并重置到开始
- **▶ 播放**: 正常回放
- **⏭ 前进**: 步进前进一帧

---

## 4. 数据结构设计

### 4.1 发送数据结构

```cpp
// 单个发送帧配置
struct FrameSendConfig {
    bool enabled;              // 是否启用
    QString device;            // 设备名称（如"COM3"）
    int channel;               // 通道号（0=CAN0, 1=CAN1）
    uint32_t id;               // 帧ID
    bool isExtended;           // 是否扩展帧
    QByteArray data;           // 数据字节
    int interval;              // 发送间隔（ms），0=单次
    int maxCount;              // 最大发送次数，-1=无限
    int currentCount;          // 当前已发送次数
    
    // 自增配置
    bool idIncreaseEnabled;    // ID自增使能
    uint32_t idIncreaseStart;  // ID起始值
    uint32_t idIncreaseStep;   // ID步长
    uint32_t idIncreaseMax;    // ID最大值
    uint32_t currentId;        // 当前ID
    
    bool dataIncreaseEnabled;  // 数据自增使能
    int dataIncreaseByteIndex; // 自增字节索引
    uint8_t dataIncreaseStart; // 起始值
    uint8_t dataIncreaseStep;  // 步长
    uint8_t dataIncreaseMax;   // 最大值
};

// 发送定时器状态
struct SendTimerState {
    int msCounter;             // 毫秒计数器
    QElapsedTimer elapsedTimer;// 精确计时器
};
```

### 4.2 回放数据结构

```cpp
// 回放序列项
struct PlaybackSequenceItem {
    QString filename;          // 文件名或"<实时捕获数据>"
    QVector<CANFrame> frames;  // 帧数据
    QHash<int, bool> idFilters;// ID过滤器（ID -> 是否通过）
    int maxLoops;              // 最大循环次数
    int currentLoopCount;      // 当前循环计数
};

// 回放状态
struct PlaybackState {
    bool isPlaying;            // 是否正在播放
    bool isPaused;             // 是否暂停
    bool isForward;            // 是否正向播放
    int currentSeqIndex;       // 当前序列索引
    int currentFrameIndex;     // 当前帧索引
    quint64 currentTimestamp;  // 当前时间戳（微秒）
    
    // 回放设置
    bool useOriginalTiming;    // 使用原始时间戳
    bool loopSequence;         // 循环整个序列
    bool waitForTraffic;       // 等待总线流量
    double playbackSpeed;      // 回放倍速（1.0 = 正常）
    int stepInterval;          // 步进间隔（ms）
    bool isStepMode;           // 是否步进模式
};
```

---

## 5. 核心逻辑设计

### 5.1 简单发送逻辑

**定时器驱动发送**：
```cpp
void FrameSenderWidget::onTimerTick() {
    int elapsed = m_elapsedTimer.restart();
    if (elapsed == 0) elapsed = 1;
    
    for (FrameSendConfig &config : m_sendConfigs) {
        if (!config.enabled) continue;
        if (config.maxCount > 0 && config.currentCount >= config.maxCount)
            continue;  // 已达到最大次数
        
        // 累积计数
        config.msCounter += elapsed;
        
        // 检查是否到达发送时间
        if (config.msCounter >= config.interval) {
            config.msCounter = 0;
            
            // 构造帧
            CANFrame frame;
            frame.setFrameId(config.currentId);
            frame.setExtendedFrameFormat(config.isExtended);
            frame.setPayload(config.data);
            frame.bus = config.channel;
            
            // 发送
            ConnectionManager::instance()->sendFrame(config.device, frame);
            
            // 更新计数
            config.currentCount++;
            
            // ID自增
            if (config.idIncreaseEnabled) {
                config.currentId += config.idIncreaseStep;
                if (config.currentId > config.idIncreaseMax) {
                    config.currentId = config.idIncreaseStart;
                }
            }
            
            // 数据自增
            if (config.dataIncreaseEnabled) {
                int idx = config.dataIncreaseByteIndex;
                uint8_t value = config.data[idx];
                value += config.dataIncreaseStep;
                if (value > config.dataIncreaseMax) {
                    value = config.dataIncreaseStart;
                }
                config.data[idx] = value;
            }
            
            // 更新UI
            updateTableRow(index);
        }
    }
}
```

**关键点**：
1. 使用1ms精确定时器（Qt::PreciseTimer）
2. 累积计数补偿定时器误差
3. 批量发送优化性能
4. 独立的ID和数据自增逻辑

### 5.2 回放逻辑

**正常回放（使用原始时间戳）**：
```cpp
void FramePlaybackWidget::onPlaybackTimerTick() {
    if (!m_playbackState.isPlaying) return;
    
    // 获取经过的时间（微秒）
    quint64 elapsed = m_elapsedTimer.nsecsElapsed() / 1000;
    m_elapsedTimer.start();
    
    // 更新当前时间戳
    if (m_playbackState.isForward) {
        m_playbackState.currentTimestamp += elapsed * m_playbackState.playbackSpeed;
    } else {
        if (m_playbackState.currentTimestamp > elapsed * m_playbackState.playbackSpeed)
            m_playbackState.currentTimestamp -= elapsed * m_playbackState.playbackSpeed;
        else
            m_playbackState.currentTimestamp = 0;
    }
    
    // 发送所有时间戳小于等于当前时间的帧
    QList<CANFrame> sendBuffer;
    PlaybackSequenceItem &seq = m_sequences[m_playbackState.currentSeqIndex];
    
    while (m_playbackState.currentFrameIndex < seq.frames.size()) {
        const CANFrame &frame = seq.frames[m_playbackState.currentFrameIndex];
        quint64 frameTime = frame.timeStamp().microSeconds();
        
        if (m_playbackState.isForward) {
            if (frameTime > m_playbackState.currentTimestamp) break;
        } else {
            if (frameTime < m_playbackState.currentTimestamp) break;
        }
        
        // 应用ID过滤
        if (!seq.idFilters.value(frame.frameId(), false)) {
            m_playbackState.currentFrameIndex++;
            continue;
        }
        
        // 添加到发送缓冲
        CANFrame sendFrame = frame;
        sendFrame.bus = m_currentChannel;  // 应用通道映射
        sendBuffer.append(sendFrame);
        
        m_playbackState.currentFrameIndex++;
    }
    
    // 批量发送
    if (!sendBuffer.isEmpty()) {
        ConnectionManager::instance()->sendFrames(m_currentDevice, sendBuffer);
    }
    
    // 检查是否到达序列末尾
    if (m_playbackState.currentFrameIndex >= seq.frames.size()) {
        onSequenceEnd();
    }
    
    // 更新UI
    updateProgress();
}
```

**步进回放**：
```cpp
void FramePlaybackWidget::onStepForward() {
    if (m_playbackState.currentSeqIndex < 0) return;
    
    PlaybackSequenceItem &seq = m_sequences[m_playbackState.currentSeqIndex];
    
    if (m_playbackState.currentFrameIndex >= seq.frames.size()) {
        // 到达末尾，移动到下一个序列
        onSequenceEnd();
        return;
    }
    
    // 发送当前帧
    const CANFrame &frame = seq.frames[m_playbackState.currentFrameIndex];
    
    // 应用ID过滤
    if (seq.idFilters.value(frame.frameId(), false)) {
        CANFrame sendFrame = frame;
        sendFrame.bus = m_currentChannel;
        ConnectionManager::instance()->sendFrame(m_currentDevice, sendFrame);
    }
    
    m_playbackState.currentFrameIndex++;
    updateProgress();
}
```

**序列结束处理**：
```cpp
void FramePlaybackWidget::onSequenceEnd() {
    PlaybackSequenceItem &seq = m_sequences[m_playbackState.currentSeqIndex];
    
    // 增加循环计数
    seq.currentLoopCount++;
    
    // 检查是否需要继续循环当前序列
    if (seq.currentLoopCount < seq.maxLoops) {
        // 重置到序列开始
        m_playbackState.currentFrameIndex = 0;
        if (seq.frames.size() > 0) {
            m_playbackState.currentTimestamp = seq.frames[0].timeStamp().microSeconds();
        }
        return;
    }
    
    // 当前序列循环完成，移动到下一个序列
    seq.currentLoopCount = 0;
    m_playbackState.currentSeqIndex++;
    
    // 检查是否到达序列列表末尾
    if (m_playbackState.currentSeqIndex >= m_sequences.size()) {
        if (m_playbackState.loopSequence) {
            // 循环整个序列列表
            m_playbackState.currentSeqIndex = 0;
        } else {
            // 停止回放
            stopPlayback();
            return;
        }
    }
    
    // 初始化新序列
    m_playbackState.currentFrameIndex = 0;
    PlaybackSequenceItem &newSeq = m_sequences[m_playbackState.currentSeqIndex];
    if (newSeq.frames.size() > 0) {
        m_playbackState.currentTimestamp = newSeq.frames[0].timeStamp().microSeconds();
    }
}
```

---

## 6. 文件格式支持

### 6.1 配置文件格式（JSON）

**发送配置 (sender_config.json)**：
```json
{
    "version": "1.0",
    "device": "COM3",
    "channel": 0,
    "frames": [
        {
            "enabled": true,
            "id": "0x100",
            "extended": false,
            "data": "00 11 22 33 44 55 66 77",
            "interval": 100,
            "maxCount": 10,
            "idIncrease": {
                "enabled": false,
                "start": "0x100",
                "step": 1,
                "max": "0x1FF"
            },
            "dataIncrease": {
                "enabled": true,
                "byteIndex": 0,
                "start": 0,
                "step": 1,
                "max": 255
            }
        }
    ]
}
```

**回放配置 (playback_config.json)**：
```json
{
    "version": "1.0",
    "device": "COM3",
    "channel": 0,
    "sequences": [
        {
            "filename": "test_data.blf",
            "loops": 1,
            "idFilters": {
                "0x100": true,
                "0x200": true,
                "0x300": false
            }
        }
    ],
    "settings": {
        "useOriginalTiming": true,
        "loopSequence": false,
        "playbackSpeed": 1.0,
        "stepInterval": 10
    }
}
```

### 6.2 支持的回放文件格式

**BLF (Binary Logging Format)** - 优先级P0
- Vector工具标准格式
- 二进制，高效
- 需要解析库

**ASC (ASCII Logging Format)** - 优先级P1
- 文本格式，易读
- 格式示例：
```
0.000000 1 100 Rx d 8 00 11 22 33 44 55 66 77
0.010000 1 200 Tx d 8 AA BB CC DD EE FF 00 11
```

**CSV (Comma-Separated Values)** - 优先级P2
- 通用格式
- 格式示例：
```
Time,Channel,ID,Dir,DLC,Data
0.000000,1,0x100,Rx,8,00 11 22 33 44 55 66 77
0.010000,1,0x200,Tx,8,AA BB CC DD EE FF 00 11
```

---

## 7. 架构集成

### 7.1 类关系图

```
┌─────────────────────────────────────────────────────────────┐
│                      MainWindow                              │
│  - 创建发送/回放窗口                                         │
│  - 管理窗口生命周期                                          │
└─────────────────────────────────────────────────────────────┘
                    ↓                    ↓
    ┌───────────────────────┐  ┌───────────────────────────┐
    │  FrameSenderWidget    │  │  FramePlaybackWidget      │
    │  - UI管理             │  │  - UI管理                 │
    │  - 定时器             │  │  - 序列管理               │
    │  - 配置管理           │  │  - 回放控制               │
    └───────────────────────┘  └───────────────────────────┘
                    ↓                    ↓
    ┌───────────────────────┐  ┌───────────────────────────┐
    │  FrameSenderCore      │  │  FramePlaybackCore        │
    │  - 发送逻辑           │  │  - 回放逻辑               │
    │  - 自增处理           │  │  - 时间戳处理             │
    │  - 定时器管理         │  │  - 文件解析               │
    └───────────────────────┘  └───────────────────────────┘
                    ↓                    ↓
            ┌───────────────────────────────────┐
            │     ConnectionManager (单例)      │
            │  - sendFrame()                    │
            │  - sendFrames()                   │
            └───────────────────────────────────┘
```

### 7.2 文件组织（遵守500行限制）

```
ZCANPRO/src/ui/
├── FrameSenderWidget.h          (80行)
├── FrameSenderWidget.cpp        (300行)
├── FrameSenderCore.h            (60行)
├── FrameSenderCore.cpp          (250行)
├── FramePlaybackWidget.h        (90行)
├── FramePlaybackWidget.cpp      (350行)
├── FramePlaybackCore.h          (70行)
├── FramePlaybackCore.cpp        (300行)
└── FrameFileParser.cpp          (200行)
```

---

## 8. 实施计划

### 阶段1：简单发送（3-4天）

**Day 1-2: 基础UI和周期发送**
- [ ] 创建FrameSenderWidget窗口
- [ ] 实现表格UI（QTableWidget）
- [ ] 实现基础周期发送
- [ ] 集成ConnectionManager

**Day 3: ID/数据自增**
- [ ] 实现ID自增逻辑
- [ ] 实现数据自增逻辑
- [ ] 添加高级选项UI

**Day 4: 配置保存/加载**
- [ ] 实现JSON配置保存
- [ ] 实现JSON配置加载
- [ ] 测试和Bug修复

### 阶段2：回放功能（4-5天）

**Day 1-2: 基础回放UI**
- [ ] 创建FramePlaybackWidget窗口
- [ ] 实现序列列表UI
- [ ] 实现控制按钮

**Day 3: 文件解析**
- [ ] 实现BLF文件解析
- [ ] 实现ASC文件解析
- [ ] 加载实时捕获数据

**Day 4: 回放逻辑**
- [ ] 实现正常回放
- [ ] 实现倍速控制
- [ ] 实现步进回放

**Day 5: ID过滤和配置**
- [ ] 实现ID过滤UI
- [ ] 实现配置保存/加载
- [ ] 测试和Bug修复

### 阶段3：优化与扩展（2-3天）

**Day 1: 性能优化**
- [ ] 批量发送优化
- [ ] 内存管理优化
- [ ] UI响应优化

**Day 2: 用户体验**
- [ ] 添加进度指示
- [ ] 添加错误提示
- [ ] 完善快捷键

**Day 3: 文档和测试**
- [ ] 编写用户文档
- [ ] 压力测试
- [ ] Bug修复

---

## 9. 测试计划

### 9.1 功能测试

**简单发送**：
- [ ] 单帧周期发送
- [ ] 多帧同时发送
- [ ] ID自增正确性
- [ ] 数据自增正确性
- [ ] 次数限制正确性
- [ ] 配置保存/加载

**回放功能**：
- [ ] BLF文件回放
- [ ] ASC文件回放
- [ ] 倍速回放准确性
- [ ] 步进回放正确性
- [ ] ID过滤正确性
- [ ] 序列循环正确性

### 9.2 性能测试

- [ ] 1000帧/秒持续发送
- [ ] 10000帧文件回放
- [ ] 多窗口同时运行
- [ ] 长时间运行稳定性（1小时+）

### 9.3 边界测试

- [ ] 空配置处理
- [ ] 无效文件处理
- [ ] 设备断开处理
- [ ] 内存不足处理

---

**文档版本**: v1.0  
**创建日期**: 2026-02-25  
**作者**: CANAnalyzerPro Team
