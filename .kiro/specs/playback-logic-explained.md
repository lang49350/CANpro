# 回放功能核心逻辑详解

## 1. 回放功能的本质

回放功能就是**按照记录文件中的时间戳，重新发送CAN帧到总线上**。

### 1.1 核心概念

```
记录文件:
Time(us)  | ID    | Data
----------|-------|------------------
0         | 0x100 | 00 11 22 33 44 55 66 77
10000     | 0x200 | AA BB CC DD EE FF 00 11
20000     | 0x100 | 01 12 23 34 45 56 67 78
30000     | 0x300 | FF FF FF FF FF FF FF FF

回放过程:
t=0ms    → 发送 0x100
t=10ms   → 发送 0x200
t=20ms   → 发送 0x100
t=30ms   → 发送 0x300
```

---

## 2. 三种回放模式

### 2.1 正常回放（使用原始时间戳）

**原理**：严格按照文件中记录的时间间隔发送

```cpp
// 伪代码
currentTime = 0;
for each frame in file:
    waitUntil(currentTime >= frame.timestamp);
    sendFrame(frame);
    currentTime = frame.timestamp;
```

**实现要点**：
- 使用高精度定时器（QElapsedTimer）
- 累积经过的时间
- 当累积时间 >= 帧时间戳时发送

**示例**：
```
文件: 0ms, 10ms, 20ms, 30ms
回放: 等0ms发送 → 等10ms发送 → 等10ms发送 → 等10ms发送
实际: 完全按照原始时间间隔
```

### 2.2 倍速回放

**原理**：按倍率加速或减速时间流逝

```cpp
// 伪代码
speedMultiplier = 2.0;  // 2倍速
currentTime = 0;
for each frame in file:
    adjustedTime = frame.timestamp / speedMultiplier;
    waitUntil(currentTime >= adjustedTime);
    sendFrame(frame);
    currentTime = adjustedTime;
```

**示例**：
```
文件: 0ms, 10ms, 20ms, 30ms
2倍速: 0ms, 5ms, 10ms, 15ms  (时间间隔减半)
0.5倍速: 0ms, 20ms, 40ms, 60ms  (时间间隔加倍)
```

### 2.3 尽可能快回放

**原理**：忽略时间戳，全速发送

```cpp
// 伪代码
for each frame in file:
    sendFrame(frame);  // 立即发送，不等待
```

**用途**：
- 压力测试
- 快速验证数据
- 不关心时间精度的场景

---

## 3. 步进回放

**原理**：每次只发送一帧或一段时间内的帧，然后暂停

### 3.1 单帧步进

```cpp
void stepForward() {
    if (currentIndex < frames.size()) {
        sendFrame(frames[currentIndex]);
        currentIndex++;
    }
}
```

**用途**：逐帧观察，调试

### 3.2 时间窗口步进（ZLG特色）

```cpp
void stepForwardByTime(int intervalMs) {
    quint64 endTime = currentTime + intervalMs * 1000;  // 转微秒
    
    while (currentIndex < frames.size()) {
        if (frames[currentIndex].timestamp > endTime) {
            break;  // 超出时间窗口，停止
        }
        sendFrame(frames[currentIndex]);
        currentIndex++;
    }
    
    currentTime = endTime;
}
```

**示例**：
```
文件: 0ms, 5ms, 10ms, 15ms, 20ms, 25ms, 30ms
步进间隔: 10ms

步进1: 发送 0ms, 5ms, 10ms (时间窗口 0-10ms)
步进2: 发送 15ms, 20ms (时间窗口 10-20ms)
步进3: 发送 25ms, 30ms (时间窗口 20-30ms)
```

**用途**：
- 观察一段时间内的帧交互
- 比单帧步进更高效
- 调试复杂协议

---

## 4. 序列回放

**原理**：支持多个文件按顺序回放，每个文件可以循环

### 4.1 数据结构

```cpp
struct SequenceItem {
    QString filename;
    QVector<CANFrame> frames;
    int maxLoops;           // 循环次数
    int currentLoopCount;   // 当前循环计数
};

QList<SequenceItem> sequences;
int currentSeqIndex = 0;
```

### 4.2 回放流程

```
序列列表:
1. file1.blf (循环2次)
2. file2.asc (循环1次)
3. file3.blf (循环3次)

回放过程:
1. 播放 file1.blf 第1次
2. 播放 file1.blf 第2次
3. 播放 file2.asc 第1次
4. 播放 file3.blf 第1次
5. 播放 file3.blf 第2次
6. 播放 file3.blf 第3次
7. 结束（或循环整个序列）
```

### 4.3 实现逻辑

```cpp
void onSequenceEnd() {
    SequenceItem &seq = sequences[currentSeqIndex];
    seq.currentLoopCount++;
    
    if (seq.currentLoopCount < seq.maxLoops) {
        // 继续循环当前文件
        currentFrameIndex = 0;
        return;
    }
    
    // 当前文件循环完成，移动到下一个
    seq.currentLoopCount = 0;
    currentSeqIndex++;
    
    if (currentSeqIndex >= sequences.size()) {
        if (loopEntireSequence) {
            currentSeqIndex = 0;  // 循环整个序列
        } else {
            stopPlayback();  // 停止
        }
    }
    
    currentFrameIndex = 0;
}
```

---

## 5. ID过滤

**原理**：只回放选中的ID，忽略其他ID

### 5.1 过滤器数据结构

```cpp
QHash<int, bool> idFilters;  // ID -> 是否通过

// 示例
idFilters[0x100] = true;   // 通过
idFilters[0x200] = true;   // 通过
idFilters[0x300] = false;  // 不通过
```

### 5.2 应用过滤

```cpp
void sendFrameWithFilter(const CANFrame &frame) {
    // 检查ID是否在过滤器中
    if (!idFilters.contains(frame.frameId())) {
        return;  // ID不在列表中，跳过
    }
    
    // 检查是否通过过滤
    if (!idFilters[frame.frameId()]) {
        return;  // 过滤器设置为false，跳过
    }
    
    // 通过过滤，发送
    sendFrame(frame);
}
```

---

## 6. 通道映射（ZLG特色）

**原理**：将文件中的通道映射到实际设备的通道

### 6.1 应用场景

```
场景1: 单通道设备回放双通道文件
文件: CAN0 + CAN1
设备: 只有CAN0
映射: CAN0 → CAN0, CAN1 → CAN0

场景2: 通道交换
文件: CAN0 + CAN1
设备: CAN0 + CAN1
映射: CAN0 → CAN1, CAN1 → CAN0
```

### 6.2 实现

```cpp
QMap<int, int> channelMapping;  // 文件通道 → 设备通道

// 配置映射
channelMapping[0] = 0;  // 文件CAN0 → 设备CAN0
channelMapping[1] = 0;  // 文件CAN1 → 设备CAN0

// 应用映射
void sendFrameWithMapping(CANFrame frame) {
    int fileChannel = frame.bus;
    int deviceChannel = channelMapping.value(fileChannel, fileChannel);
    frame.bus = deviceChannel;
    sendFrame(frame);
}
```

---

## 7. 定时器实现细节

### 7.1 为什么使用1ms定时器？

```cpp
QTimer *timer = new QTimer();
timer->setTimerType(Qt::PreciseTimer);
timer->setInterval(1);  // 1ms
```

**原因**：
- 1ms是足够高的精度（大多数CAN总线周期 >= 10ms）
- 更小的间隔（如0.1ms）会增加CPU负担
- 通过累积计数补偿定时器误差

### 7.2 累积计数补偿误差

```cpp
QElapsedTimer elapsedTimer;
quint64 currentTimestamp = 0;

void onTimerTick() {
    // 获取实际经过的时间（微秒）
    quint64 elapsed = elapsedTimer.nsecsElapsed() / 1000;
    elapsedTimer.restart();
    
    // 累积到当前时间戳
    currentTimestamp += elapsed;
    
    // 发送所有时间戳 <= currentTimestamp 的帧
    while (currentFrameIndex < frames.size()) {
        if (frames[currentFrameIndex].timestamp > currentTimestamp) {
            break;
        }
        sendFrame(frames[currentFrameIndex]);
        currentFrameIndex++;
    }
}
```

**优点**：
- 即使定时器不精确（如2ms才触发一次），累积时间仍然准确
- 避免时间漂移
- 自动补偿系统延迟

---

## 8. 倒放实现

**原理**：从后往前遍历帧列表

```cpp
void playBackward() {
    quint64 currentTimestamp = frames[currentFrameIndex].timestamp;
    
    while (currentFrameIndex > 0) {
        currentFrameIndex--;
        
        // 计算时间差
        quint64 prevTimestamp = frames[currentFrameIndex].timestamp;
        quint64 timeDiff = currentTimestamp - prevTimestamp;
        
        // 等待时间差
        wait(timeDiff);
        
        // 发送帧
        sendFrame(frames[currentFrameIndex]);
        
        currentTimestamp = prevTimestamp;
    }
}
```

**注意**：
- 倒放时时间戳是递减的
- 需要计算相邻帧的时间差
- 到达开头时处理序列切换

---

## 9. 性能优化

### 9.1 批量发送

```cpp
// ❌ 错误：逐个发送
for (frame : framesToSend) {
    ConnectionManager::sendFrame(frame);  // 每次都有开销
}

// ✅ 正确：批量发送
QList<CANFrame> buffer;
for (frame : framesToSend) {
    buffer.append(frame);
}
ConnectionManager::sendFrames(buffer);  // 一次发送
```

### 9.2 预加载和排序

```cpp
// 加载文件时
QVector<CANFrame> frames = loadFile(filename);

// 按时间戳排序（确保顺序正确）
std::sort(frames.begin(), frames.end());

// 预处理ID过滤器
QHash<int, bool> idFilters;
for (const CANFrame &frame : frames) {
    if (!idFilters.contains(frame.frameId())) {
        idFilters[frame.frameId()] = true;  // 默认通过
    }
}
```

---

## 10. 关键代码片段

### 10.1 完整的回放定时器

```cpp
void FramePlaybackWidget::onPlaybackTimerTick() {
    if (!m_isPlaying) return;
    
    // 1. 获取经过时间
    quint64 elapsed = m_elapsedTimer.nsecsElapsed() / 1000;  // 微秒
    m_elapsedTimer.restart();
    
    // 2. 应用倍速
    elapsed = elapsed * m_playbackSpeed;
    
    // 3. 更新当前时间戳
    if (m_isForward) {
        m_currentTimestamp += elapsed;
    } else {
        if (m_currentTimestamp > elapsed)
            m_currentTimestamp -= elapsed;
        else
            m_currentTimestamp = 0;
    }
    
    // 4. 收集要发送的帧
    QList<CANFrame> sendBuffer;
    PlaybackSequenceItem &seq = m_sequences[m_currentSeqIndex];
    
    while (m_currentFrameIndex < seq.frames.size()) {
        const CANFrame &frame = seq.frames[m_currentFrameIndex];
        quint64 frameTime = frame.timeStamp().microSeconds();
        
        // 检查时间戳
        if (m_isForward) {
            if (frameTime > m_currentTimestamp) break;
        } else {
            if (frameTime < m_currentTimestamp) break;
        }
        
        // 应用ID过滤
        if (!seq.idFilters.value(frame.frameId(), false)) {
            m_currentFrameIndex++;
            continue;
        }
        
        // 应用通道映射
        CANFrame sendFrame = frame;
        sendFrame.bus = m_channelMapping.value(frame.bus, frame.bus);
        
        sendBuffer.append(sendFrame);
        m_currentFrameIndex++;
    }
    
    // 5. 批量发送
    if (!sendBuffer.isEmpty()) {
        ConnectionManager::instance()->sendFrames(m_currentDevice, sendBuffer);
    }
    
    // 6. 检查序列结束
    if (m_currentFrameIndex >= seq.frames.size()) {
        onSequenceEnd();
    }
    
    // 7. 更新UI
    updateProgress();
}
```

---

## 11. 总结

### 核心要点

1. **时间戳是关键**：所有回放都围绕时间戳进行
2. **累积计数补偿误差**：使用QElapsedTimer精确计时
3. **批量发送优化性能**：减少函数调用开销
4. **序列管理支持复杂场景**：多文件、循环、过滤
5. **步进回放方便调试**：逐帧或时间窗口观察

### 实现难点

1. **定时器精度**：Windows定时器不够精确，需要累积补偿
2. **时间戳处理**：微秒级精度，避免溢出
3. **序列切换**：正确处理循环和序列结束
4. **性能优化**：高速回放时避免UI卡顿

### 建议实施顺序

1. **先实现正常回放**：使用原始时间戳
2. **再添加倍速**：简单的乘法运算
3. **然后步进回放**：暂停和单步前进
4. **最后序列管理**：多文件和循环

---

**文档版本**: v1.0  
**创建日期**: 2026-02-25  
**作者**: CANAnalyzerPro Team
