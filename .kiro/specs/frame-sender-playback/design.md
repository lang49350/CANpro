# 帧发送与回放功能设计文档

## 1. 设计概述

本文档详细设计ZCANPRO上位机的帧发送和回放功能的技术实现方案。

**菜单整合方案**：
- 在MainWindow的"发送数据"菜单中添加"数据回放"菜单项
- 移除原有的"单帧发送"菜单项
- 菜单结构：发送数据 → 数据回放、周期发送、发送列表、发送脚本

**设计原则**：
- 遵循ZCANPRO现有架构（ConnectionManager、DataRouter）
- 严格遵守500行文件限制
- 单一职责原则
- 分阶段实现，先MVP后扩展

**参考资料**：
- `#[[file:can-frame-sender-analysis.md]]` - SavvyCAN和ZLG功能分析
- `#[[file:playback-logic-explained.md]]` - 回放逻辑详细说明
- `#[[file:../docs/Architecture.md]]` - ZCANPRO架构文档

---

## 2. 架构设计

### 2.1 整体架构

```
┌─────────────────────────────────────────────────────────────┐
│                      MainWindow                              │
│  - "发送数据"菜单                                            │
│    ├─ 数据回放 (替换原"单帧发送")                           │
│    ├─ 周期发送 (暂时禁用)                                    │
│    ├─ 发送列表 (暂时禁用)                                    │
│    └─ 发送脚本 (暂时禁用)                                    │
│  - QMdiArea (MDI区域)                                        │
│    └─ CustomMdiSubWindow                                     │
│       └─ FramePlaybackWidget (嵌入式窗口)                   │
└─────────────────────────────────────────────────────────────┘
                    ↓
    ┌───────────────────────────────────────────────────┐
    │  FramePlaybackWidget (嵌入MDI，类似CANViewWidget) │
    │  - UI管理                                          │
    │  - 序列管理                                        │
    │  - 回放控制                                        │
    └───────────────────────────────────────────────────┘
                    ↓
    ┌───────────────────────────────────────────────────┐
    │  FramePlaybackCore                                 │
    │  - 回放逻辑                                        │
    │  - 时间戳处理                                      │
    │  - 文件解析 (BLF/ASC)                             │
    └───────────────────────────────────────────────────┘
                    ↓
            ┌───────────────────────────────────┐
            │     ConnectionManager (单例)      │
            │  - sendFrame()                    │
            │  - sendFrames()                   │
            └───────────────────────────────────┘
```

### 2.2 菜单整合实现

**MainWindow.cpp 修改**：
```cpp
// 在 MainWindow::createToolbar() 中修改"发送数据"菜单
QMenu *menuSend = new QMenu(btnSend);
menuSend->setStyleSheet(menuStyle);

// 用"数据回放"替换"单帧发送"
QAction *actPlayback = menuSend->addAction("数据回放", this, &MainWindow::onDataPlayback);

// 暂时禁用其他功能
QAction *actPeriodic = menuSend->addAction("周期发送");
actPeriodic->setEnabled(false);  // 禁用

QAction *actList = menuSend->addAction("发送列表");
actList->setEnabled(false);  // 禁用

QAction *actScript = menuSend->addAction("发送脚本");
actScript->setEnabled(false);  // 禁用
```

**新增槽函数（参考onNewCANView）**：
```cpp
void MainWindow::onDataPlayback()
{
    qDebug() << "📡 创建数据回放窗口";
    
    // 检查窗口数量限制（可选）
    if (m_playbackWindowCount >= 5) {  // 限制最多5个回放窗口
        QMessageBox::warning(this, "提示", 
            "最多只能创建5个回放窗口\n"
            "当前已有5个窗口，请关闭一些窗口后再试");
        return;
    }
    
    // 创建FramePlaybackWidget
    FramePlaybackWidget *playbackWidget = new FramePlaybackWidget();
    
    // 使用自定义MDI子窗口
    CustomMdiSubWindow *subWindow = new CustomMdiSubWindow();
    subWindow->setWidget(playbackWidget);
    
    // 设置窗口标题（使用编号）
    int windowId = m_playbackWindowNextId;
    subWindow->setWindowTitle(QString("回放%1").arg(windowId));
    
    // 增加窗口数量和编号
    m_playbackWindowCount++;
    m_playbackWindowNextId++;
    
    // 添加到MDI区域
    m_mdiArea->addSubWindow(subWindow);
    subWindow->resize(900, 700);
    subWindow->show();
    
    // 监听窗口销毁事件
    connect(subWindow, &QObject::destroyed, this, [this]() {
        m_playbackWindowCount--;
        qDebug() << "🗑 回放窗口已销毁，当前回放窗口数:" << m_playbackWindowCount;
        QTimer::singleShot(100, this, &MainWindow::arrangeSubWindows);
    });
    
    // 延迟自动排列窗口
    m_resizeTimer->start(100);
}
```

### 2.3 类职责划分

**FrameSenderWidget** (UI层，预计300行)
- 表格UI创建和管理
- 用户交互处理
- 配置文件保存/加载
- 调用FrameSenderCore执行发送

**FrameSenderCore** (逻辑层，预计250行)
- 定时器管理（1ms精确定时器）
- 发送逻辑实现
- ID/数据自增处理
- 统计计数


**FramePlaybackWidget** (UI层，预计350行)
- 嵌入MDI区域（类似CANViewWidget）
- 工具栏UI（设备/通道选择）
- 序列列表管理
- 回放控制按钮
- ID过滤UI
- 循环回放勾选框
- 调用FramePlaybackCore执行回放

**FramePlaybackCore** (逻辑层，预计300行)
- 回放定时器管理
- 时间戳处理
- 倍速控制
- 步进回放逻辑
- 序列切换
- 文件解析（BLF/ASC优先）

**FrameFileParser** (工具类，预计200行)
- BLF文件解析（优先级P0）
- ASC文件解析（优先级P0）
- CSV文件解析（优先级P2，可选）

---

## 3. 数据结构设计

### 3.1 发送数据结构

```cpp
/**
 * @brief 单个发送帧配置
 */
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

/**
 * @brief 发送定时器状态
 */
struct SendTimerState {
    int msCounter;             // 毫秒计数器
    QElapsedTimer elapsedTimer;// 精确计时器
};
```

### 3.2 回放数据结构

```cpp
/**
 * @brief 回放序列项
 */
struct PlaybackSequenceItem {
    QString filename;          // 文件名或"<实时捕获数据>"
    QVector<CANFrame> frames;  // 帧数据
    QHash<int, bool> idFilters;// ID过滤器（ID -> 是否通过）
    int maxLoops;              // 最大循环次数
    int currentLoopCount;      // 当前循环计数
};

/**
 * @brief 回放状态
 */
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

## 4. 核心算法设计

### 4.1 定时器驱动发送算法

**原理**：使用1ms精确定时器，通过累积计数补偿定时器误差


```cpp
void FrameSenderCore::onTimerTick() {
    // 1. 获取实际经过的时间
    int elapsed = m_elapsedTimer.restart();
    if (elapsed == 0) elapsed = 1;
    
    // 2. 遍历所有发送配置
    for (FrameSendConfig &config : m_sendConfigs) {
        if (!config.enabled) continue;
        if (config.maxCount > 0 && config.currentCount >= config.maxCount)
            continue;  // 已达到最大次数
        
        // 3. 累积计数（补偿定时器误差）
        config.msCounter += elapsed;
        
        // 4. 检查是否到达发送时间
        if (config.msCounter >= config.interval) {
            config.msCounter = 0;
            
            // 5. 构造帧
            CANFrame frame;
            frame.setFrameId(config.currentId);
            frame.setExtendedFrameFormat(config.isExtended);
            frame.setPayload(config.data);
            frame.bus = config.channel;
            
            // 6. 发送
            ConnectionManager::instance()->sendFrame(config.device, frame);
            
            // 7. 更新计数
            config.currentCount++;
            
            // 8. ID自增
            if (config.idIncreaseEnabled) {
                config.currentId += config.idIncreaseStep;
                if (config.currentId > config.idIncreaseMax) {
                    config.currentId = config.idIncreaseStart;
                }
            }
            
            // 9. 数据自增
            if (config.dataIncreaseEnabled) {
                int idx = config.dataIncreaseByteIndex;
                uint8_t value = config.data[idx];
                value += config.dataIncreaseStep;
                if (value > config.dataIncreaseMax) {
                    value = config.dataIncreaseStart;
                }
                config.data[idx] = value;
            }
        }
    }
}
```

**关键点**：
1. 使用QElapsedTimer获取精确的经过时间
2. 累积计数补偿定时器误差（即使定时器2ms才触发一次，累积时间仍然准确）
3. 独立的ID和数据自增逻辑
4. 批量发送优化（可选）

### 4.2 回放时间戳处理算法

**原理**：累积经过时间，发送所有时间戳小于等于当前时间的帧

```cpp
void FramePlaybackCore::onPlaybackTimerTick() {
    if (!m_playbackState.isPlaying) return;
    
    // 1. 获取经过的时间（微秒）
    quint64 elapsed = m_elapsedTimer.nsecsElapsed() / 1000;
    m_elapsedTimer.start();
    
    // 2. 应用倍速
    elapsed = elapsed * m_playbackState.playbackSpeed;
    
    // 3. 更新当前时间戳
    if (m_playbackState.isForward) {
        m_playbackState.currentTimestamp += elapsed;
    } else {
        if (m_playbackState.currentTimestamp > elapsed)
            m_playbackState.currentTimestamp -= elapsed;
        else
            m_playbackState.currentTimestamp = 0;
    }
    
    // 4. 收集要发送的帧
    QList<CANFrame> sendBuffer;
    PlaybackSequenceItem &seq = m_sequences[m_playbackState.currentSeqIndex];
    
    while (m_playbackState.currentFrameIndex < seq.frames.size()) {
        const CANFrame &frame = seq.frames[m_playbackState.currentFrameIndex];
        quint64 frameTime = frame.timeStamp().microSeconds();
        
        // 检查时间戳
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
        
        // 应用通道映射
        CANFrame sendFrame = frame;
        sendFrame.bus = m_currentChannel;
        
        sendBuffer.append(sendFrame);
        m_playbackState.currentFrameIndex++;
    }
    
    // 5. 批量发送
    if (!sendBuffer.isEmpty()) {
        ConnectionManager::instance()->sendFrames(m_currentDevice, sendBuffer);
    }
    
    // 6. 检查序列结束
    if (m_playbackState.currentFrameIndex >= seq.frames.size()) {
        onSequenceEnd();
    }
}
```


**关键点**：
1. 使用微秒级时间戳精度
2. 批量发送优化性能
3. 支持正向和倒放
4. ID过滤和通道映射

### 4.3 步进回放算法

**单帧步进**：
```cpp
void FramePlaybackCore::stepForward() {
    if (m_currentFrameIndex >= m_frames.size()) return;
    
    const CANFrame &frame = m_frames[m_currentFrameIndex];
    
    // 应用过滤
    if (m_idFilters.value(frame.frameId(), false)) {
        CANFrame sendFrame = frame;
        sendFrame.bus = m_currentChannel;
        ConnectionManager::instance()->sendFrame(m_currentDevice, sendFrame);
    }
    
    m_currentFrameIndex++;
}
```

**时间窗口步进**（ZLG特色）：
```cpp
void FramePlaybackCore::stepForwardByTime(int intervalMs) {
    quint64 endTime = m_currentTimestamp + intervalMs * 1000;  // 转微秒
    
    QList<CANFrame> sendBuffer;
    while (m_currentFrameIndex < m_frames.size()) {
        const CANFrame &frame = m_frames[m_currentFrameIndex];
        if (frame.timeStamp().microSeconds() > endTime) break;
        
        if (m_idFilters.value(frame.frameId(), false)) {
            CANFrame sendFrame = frame;
            sendFrame.bus = m_currentChannel;
            sendBuffer.append(sendFrame);
        }
        m_currentFrameIndex++;
    }
    
    if (!sendBuffer.isEmpty()) {
        ConnectionManager::instance()->sendFrames(m_currentDevice, sendBuffer);
    }
    
    m_currentTimestamp = endTime;
}
```

### 4.4 序列切换算法

```cpp
void FramePlaybackCore::onSequenceEnd() {
    PlaybackSequenceItem &seq = m_sequences[m_currentSeqIndex];
    
    // 1. 增加循环计数
    seq.currentLoopCount++;
    
    // 2. 检查是否需要继续循环当前序列
    if (seq.currentLoopCount < seq.maxLoops) {
        m_currentFrameIndex = 0;
        if (seq.frames.size() > 0) {
            m_currentTimestamp = seq.frames[0].timeStamp().microSeconds();
        }
        return;
    }
    
    // 3. 当前序列循环完成，移动到下一个序列
    seq.currentLoopCount = 0;
    m_currentSeqIndex++;
    
    // 4. 检查是否到达序列列表末尾
    if (m_currentSeqIndex >= m_sequences.size()) {
        if (m_loopSequence) {
            m_currentSeqIndex = 0;  // 循环整个序列
        } else {
            stopPlayback();  // 停止回放
            return;
        }
    }
    
    // 5. 初始化新序列
    m_currentFrameIndex = 0;
    PlaybackSequenceItem &newSeq = m_sequences[m_currentSeqIndex];
    if (newSeq.frames.size() > 0) {
        m_currentTimestamp = newSeq.frames[0].timeStamp().microSeconds();
    }
}
```

---

## 5. UI设计

### 5.1 FrameSenderWidget布局

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

te slots:
    void onTimerTick();
    
private:
    QTimer *m_timer;
    QElapsedTimer m_elapsedTimer;
    QVector<FrameSendConfig> m_configs;
    int m_totalSent;
};
```

 void configUpdated(int index);
    
privaameSenderCore : public QObject {
    Q_OBJECT
public:
    explicit FrameSenderCore(QObject *parent = nullptr);
    ~FrameSenderCore();
    
    void addConfig(const FrameSendConfig &config);
    void removeConfig(int index);
    void updateConfig(int index, const FrameSendConfig &config);
    void clearConfigs();
    
    void startAll();
    void stopAll();
    void startOne(int index);
    void stopOne(int index);
    
    int getSentCount() const;
    
signals:
    void statisticsUpdated(int totalSent);
   cpp
class Fr;
    
    void loadConfig(const QString &filename);
    void saveConfig(const QString &filename);
    
private slots:
    void onAddRow();
    void onClearAll();
    void onStartAll();
    void onStopAll();
    void onLoadConfig();
    void onSaveConfig();
    void onCellChanged(int row, int col);
    
private:
    void setupUi();
    void updateStatistics();
    
    QTableWidget *m_table;
    FrameSenderCore *m_core;
    QString m_currentDevice;
    int m_currentChannel;
};
```

**FrameSenderCore.h**:
```evice);
    void setChannel(int channel)0行)
├── FrameSenderCore.h            (60行)
├── FrameSenderCore.cpp          (250行)
├── FramePlaybackWidget.h        (90行)
├── FramePlaybackWidget.cpp      (350行)
├── FramePlaybackCore.h          (70行)
├── FramePlaybackCore.cpp        (300行)
└── FrameFileParser.cpp          (200行)
```

### 6.2 类接口设计

**FrameSenderWidget.h**:
```cpp
class FrameSenderWidget : public QWidget {
    Q_OBJECT
public:
    explicit FrameSenderWidget(QWidget *parent = nullptr);
    ~FrameSenderWidget();
    
    void setDevice(const QString &didget.cpp        (30   │
│ ┌────────────────────────────────────────────────────────┐  │
│ │ [✓] 0x100 - Message_A                                   │  │
│ │ [✓] 0x200 - Message_B                                   │  │
│ │ [ ] 0x300 - Message_C                                   │  │
│ └────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────┘
```

---

## 6. 文件组织

### 6.1 文件结构（遵守500行限制）

```
ZCANPRO/src/ui/
├── FrameSenderWidget.h          (80行)
├── FrameSenderW            进  ○ 尽可能快                       │  │
│ │ 倍速: [1.0x ▼]  步进间隔: [10ms]                        │  │
│ │ [✓] 使用原始时间戳                                       │  │
│ │ [✓] 循环播放整个序列                                     │  │
│ └────────────────────────────────────────────────────────┘  │
├──────────────────────────────────────────────────────────────┤
│ ID过滤: [全选] [全不选]                       ──────────────────────┐  │
│ │ 模式: ⦿ 正常  ○ 步 [▶ 播放] [⏭ 前进]    │
│                                                              │
│ 进度: ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ │
│       45% | 1234/2700 帧 | 用时: 00:05                      │
├──────────────────────────────────────────────────────────────┤
│ 回放设置:                                                    │
│ ┌──────────────────────────────────     │
│ [⏮ 后退] [⏸ 暂停] [◀ 倒放] [⏹ 停止]| 3                     │  │
│ └────────────────────────────────────────────────────────┘  │
│ [加载文件] [删除]                                            │
├──────────────────────────────────────────────────────────────┤
│ 回放控制:                                                     │  │
│ │ another_test.asc                ──────────────┤
│ 回放序列:                                                    │
│ ┌────────────────────────────────────────────────────────┐  │
│ │ 文件名                          | 循环次数               │  │
│ │ test_data.blf                   | 1               
┌──────────────────────────────────────────────────────────────┐
│ 帧回放                                                [×]    │
├──────────────────────────────────────────────────────────────┤
│ 设备: [COM3 ▼]  通道: [CAN0 ▼]                              │
├────────────────────────────────────────────────
### 5.2 FramePlaybackWidget布局

```

**FramePlaybackWidget.h**:
```cpp
class FramePlaybackWidget : public QWidget {
    Q_OBJECT
public:
    explicit FramePlaybackWidget(QWidget *parent = nullptr);
    ~FramePlaybackWidget();
    
    void setDevice(const QString &device);
    void setChannel(int channel);
    
private slots:
    void onLoadFile();
    void onRemoveFile();
    void onPlay();
    void onPause();
    void onStop();
    void onStepForward();
    void onStepBackward();
    
private:
    void setupUi();
    void updateProgress();
    
    QListWidget *m_sequenceList;
    FramePlaybackCore *m_core;
    QString m_currentDevice;
    int m_currentChannel;
};
```

**FramePlaybackCore.h**:
```cpp
class FramePlaybackCore : public QObject {
    Q_OBJECT
public:
    explicit FramePlaybackCore(QObject *parent = nullptr);
    ~FramePlaybackCore();
    
    bool loadFile(const QString &filename);
    void addSequence(const PlaybackSequenceItem &item);
    void removeSequence(int index);
    
    void play();
    void pause();
    void stop();
    void stepForward();
    void stepBackward();
    
    void setPlaybackSpeed(double speed);
    void setStepInterval(int ms);
    void setIDFilter(int id, bool pass);
    
signals:
    void progressUpdated(int current, int total);
    void playbackFinished();
    
private slots:
    void onTimerTick();
    void onSequenceEnd();
    
private:
    QTimer *m_timer;
    QElapsedTimer m_elapsedTimer;
    QVector<PlaybackSequenceItem> m_sequences;
    PlaybackState m_state;
};
```

---

## 7. 配置文件格式

### 7.1 发送配置 (sender_config.json)

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

### 7.2 回放配置 (playback_config.json)

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

---

## 8. 性能优化策略

### 8.1 批量发送优化

```cpp
// 收集一批帧后一次性发送
QList<CANFrame> sendBuffer;
for (const FrameSendConfig &config : m_configs) {
    if (shouldSend(config)) {
        sendBuffer.append(buildFrame(config));
    }
}
if (!sendBuffer.isEmpty()) {
    ConnectionManager::instance()->sendFrames(m_currentDevice, sendBuffer);
}
```

### 8.2 预加载和排序

```cpp
// 加载文件时预处理
QVector<CANFrame> frames = FrameFileParser::loadBLF(filename);
std::sort(frames.begin(), frames.end());  // 按时间戳排序
```

### 8.3 UI刷新优化

```cpp
// 使用定时器批量刷新UI
m_uiUpdateTimer->setInterval(100);  // 100ms刷新一次
connect(m_uiUpdateTimer, &QTimer::timeout, this, &Widget::updateUI);
```

---

## 9. 错误处理

### 9.1 发送错误

```cpp
bool FrameSenderCore::sendFrame(const FrameSendConfig &config) {
    if (!ConnectionManager::instance()->isConnected(config.device)) {
        qDebug() << "❌ 设备未连接:" << config.device;
        emit errorOccurred("设备未连接");
        return false;
    }
    
    CANFrame frame = buildFrame(config);
    if (!ConnectionManager::instance()->sendFrame(config.device, frame)) {
        qDebug() << "❌ 发送失败:" << frame.frameId();
        return false;
    }
    
    return true;
}
```

### 9.2 文件解析错误

```cpp
QVector<CANFrame> FrameFileParser::loadBLF(const QString &filename) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "❌ 无法打开文件:" << filename;
        return QVector<CANFrame>();
    }
    
    // 解析文件...
    if (parseError) {
        qDebug() << "❌ 文件格式错误:" << filename;
        return QVector<CANFrame>();
    }
    
    return frames;
}
```

---

## 10. 测试策略

### 10.1 单元测试

- 定时器精度测试
- ID/数据自增逻辑测试
- 时间戳处理测试
- 序列切换逻辑测试

### 10.2 集成测试

- 与ConnectionManager集成测试
- 多设备发送测试
- 文件回放测试

### 10.3 性能测试

- 1000帧/秒持续发送
- 10000帧文件回放
- 长时间运行稳定性（1小时+）

---

**文档版本**: v1.0  
**创建日期**: 2026-02-25  
**作者**: CANAnalyzerPro Team  
**状态**: 待审核


---

## 11. 代码规范与架构约束

### 11.1 文件大小限制（强制规则）

**严格遵守500行限制**：

| 文件 | 预计行数 | 状态 |
|------|---------|------|
| FrameSenderWidget.h | 80行 | ✅ 安全 |
| FrameSenderWidget.cpp | 300行 | ✅ 安全 |
| FrameSenderCore.h | 60行 | ✅ 安全 |
| FrameSenderCore.cpp | 250行 | ✅ 安全 |
| FramePlaybackWidget.h | 90行 | ✅ 安全 |
| FramePlaybackWidget.cpp | 350行 | ✅ 安全 |
| FramePlaybackCore.h | 70行 | ✅ 安全 |
| FramePlaybackCore.cpp | 300行 | ✅ 安全 |
| FrameFileParser.cpp | 200行 | ✅ 安全 |

**超过限制时的处理**：
1. 立即停止添加新功能
2. 分析文件职责，识别可拆分的模块
3. 按功能拆分成多个文件
4. 更新文档说明拆分逻辑

**拆分示例**（如果FrameSenderWidget.cpp超过500行）：
```
FrameSenderWidget.cpp (600行) → 拆分为：
├── FrameSenderWidget.cpp (300行)      // 核心UI和交互
├── FrameSenderWidgetUI.cpp (200行)    // UI创建函数
└── FrameSenderWidgetConfig.cpp (100行) // 配置保存/加载
```

### 11.2 命名规范

#### 类名（大驼峰）
```cpp
✅ FrameSenderWidget
✅ FramePlaybackCore
✅ PlaybackSequenceItem
❌ frameSenderWidget
❌ Frame_Sender_Widget
```

#### 函数名（小驼峰）
```cpp
✅ void startAll();
✅ void onTimerTick();
✅ void setPlaybackSpeed(double speed);
❌ void StartAll();
❌ void on_timer_tick();
```

#### 成员变量（m_前缀）
```cpp
✅ QTimer *m_timer;
✅ QString m_currentDevice;
✅ QVector<FrameSendConfig> m_sendConfigs;
❌ QTimer *timer;
❌ QString currentDevice;
```

#### 常量（全大写+下划线）
```cpp
✅ const int MAX_FRAMES = 10000;
✅ const int DEFAULT_INTERVAL = 100;
❌ const int maxFrames = 10000;
❌ const int defaultInterval = 100;
```

### 11.3 注释规范

#### 文件头注释（必须）
```cpp
/**
 * @file FrameSenderWidget.cpp
 * @brief 帧发送窗口实现
 * @author CANAnalyzerPro Team
 * @date 2026-02-25
 */
```

#### 类注释（必须）
```cpp
/**
 * @brief 帧发送窗口
 * 
 * 职责：
 * - 提供帧发送的UI界面
 * - 管理发送配置表格
 * - 调用FrameSenderCore执行发送
 * 
 * 特点：
 * - 支持多帧同时发送
 * - 支持ID/数据自增
 * - 支持配置保存/加载
 */
class FrameSenderWidget : public QWidget {
```

#### 函数注释（公共接口必须）
```cpp
/**
 * @brief 设置要使用的设备
 * @param device 设备串口名称（如"COM3"）
 * 
 * @note 设备必须已连接，否则发送会失败
 */
void setDevice(const QString &device);

/**
 * @brief 启动所有已启用的发送配置
 * 
 * @warning 如果设备未连接，会发送errorOccurred信号
 */
void startAll();
```

#### 行内注释（复杂逻辑必须）
```cpp
// 累积计数补偿定时器误差
config.msCounter += elapsed;

// ID自增回绕到起始值
if (config.currentId > config.idIncreaseMax) {
    config.currentId = config.idIncreaseStart;
}
```

### 11.4 代码组织规范

#### 头文件组织
```cpp
#ifndef FRAMESENDERWIDGET_H
#define FRAMESENDERWIDGET_H

// 1. Qt头文件
#include <QWidget>
#include <QString>
#include <QTimer>

// 2. 项目头文件
#include "../core/CANFrame.h"
#include "FrameSenderCore.h"

// 3. 前向声明
class QTableWidget;
class QPushButton;

// 4. 类定义
class FrameSenderWidget : public QWidget {
    Q_OBJECT
    
public:
    // 构造/析构
    explicit FrameSenderWidget(QWidget *parent = nullptr);
    ~FrameSenderWidget();
    
    // 公共函数
    void setDevice(const QString &device);
    void setChannel(int channel);
    
public slots:
    // 公共槽函数
    void onStartAll();
    void onStopAll();
    
private slots:
    // 私有槽函数
    void onTimerTick();
    void onCellChanged(int row, int col);
    
private:
    // 私有函数
    void setupUi();
    void createTitleBar();
    void createToolbar();
    void createTable();
    void createButtonBar();
    void createStatusBar();
    
    // 成员变量
    QTableWidget *m_table;
    FrameSenderCore *m_core;
    QString m_currentDevice;
    int m_currentChannel;
};

#endif // FRAMESENDERWIDGET_H
```

#### 源文件组织
```cpp
// 1. 头文件
#include "FrameSenderWidget.h"

// 2. Qt头文件
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>

// 3. 项目头文件
#include "../core/ConnectionManager.h"

// 4. 构造/析构
FrameSenderWidget::FrameSenderWidget(QWidget *parent)
    : QWidget(parent)
    , m_table(nullptr)
    , m_core(nullptr)
    , m_currentDevice("")
    , m_currentChannel(0)
{
    setupUi();
}

FrameSenderWidget::~FrameSenderWidget()
{
    // 清理资源
}

// 5. 公共函数
void FrameSenderWidget::setDevice(const QString &device)
{
    m_currentDevice = device;
}

// 6. 公共槽函数
void FrameSenderWidget::onStartAll()
{
    // 实现
}

// 7. 私有槽函数
void FrameSenderWidget::onTimerTick()
{
    // 实现
}

// 8. 私有函数
void FrameSenderWidget::setupUi()
{
    // 实现
}
```

### 11.5 错误处理规范

#### 使用qDebug记录关键操作
```cpp
✅ qDebug() << "📡 开始发送:" << config.id;
✅ qDebug() << "⚠️ 设备未连接:" << device;
✅ qDebug() << "❌ 文件解析失败:" << filename;
✅ qDebug() << "✅ 配置加载成功:" << filename;
```

#### 使用emoji增强可读性
- ✅ 成功操作
- ⚠️ 警告信息
- ❌ 错误信息
- 📡 发送/接收
- 🔧 配置操作
- 📊 数据处理
- 📁 文件操作

#### 错误处理模式
```cpp
// ✅ 正确：检查错误并记录
if (!ConnectionManager::instance()->isConnected(device)) {
    qDebug() << "❌ 设备未连接:" << device;
    QMessageBox::critical(this, "错误", "设备未连接");
    return false;
}

// ✅ 正确：文件操作错误处理
QFile file(filename);
if (!file.open(QIODevice::WriteOnly)) {
    qDebug() << "❌ 无法创建文件:" << filename;
    QMessageBox::critical(this, "错误", "无法创建文件");
    return false;
}

// ❌ 错误：忽略错误
ConnectionManager::instance()->sendFrame(device, frame);  // 没有检查返回值
```

### 11.6 性能优化规范

#### 批量处理
```cpp
// ✅ 正确：批量发送
QList<CANFrame> sendBuffer;
for (const FrameSendConfig &config : m_configs) {
    if (shouldSend(config)) {
        sendBuffer.append(buildFrame(config));
    }
}
if (!sendBuffer.isEmpty()) {
    ConnectionManager::instance()->sendFrames(m_currentDevice, sendBuffer);
}

// ❌ 错误：逐个发送
for (const FrameSendConfig &config : m_configs) {
    if (shouldSend(config)) {
        CANFrame frame = buildFrame(config);
        ConnectionManager::instance()->sendFrame(m_currentDevice, frame);
    }
}
```

#### 避免频繁UI更新
```cpp
// ✅ 正确：使用定时器批量刷新
m_uiUpdateTimer = new QTimer(this);
m_uiUpdateTimer->setInterval(100);  // 100ms刷新一次
connect(m_uiUpdateTimer, &QTimer::timeout, this, &FrameSenderWidget::updateUI);

// ❌ 错误：每次发送都更新UI
void onFrameSent() {
    updateUI();  // 高速发送时会卡顿
}
```

#### 预加载和缓存
```cpp
// ✅ 正确：加载时预处理
QVector<CANFrame> frames = FrameFileParser::loadBLF(filename);
std::sort(frames.begin(), frames.end());  // 按时间戳排序

// 预处理ID过滤器
QHash<int, bool> idFilters;
for (const CANFrame &frame : frames) {
    if (!idFilters.contains(frame.frameId())) {
        idFilters[frame.frameId()] = true;
    }
}
```

### 11.7 内存管理规范

#### 使用父对象管理生命周期
```cpp
// ✅ 正确：指定父对象
QTimer *timer = new QTimer(this);  // 自动释放
QLabel *label = new QLabel(this);  // 自动释放

// ❌ 错误：没有父对象
QTimer *timer = new QTimer();  // 内存泄漏
```

#### 使用智能指针（可选）
```cpp
// ✅ 推荐：使用智能指针
std::unique_ptr<FrameSenderCore> m_core;
QScopedPointer<QTimer> m_timer;
```

#### 清理资源
```cpp
// ✅ 正确：析构函数清理
FrameSenderWidget::~FrameSenderWidget()
{
    if (m_timer && m_timer->isActive()) {
        m_timer->stop();
    }
    
    // Qt对象会自动清理（如果有父对象）
    // 手动分配的资源需要手动释放
}
```

### 11.8 单一职责原则

#### 类职责划分
```cpp
// ✅ 正确：职责分离
class FrameSenderWidget {
    // 只负责UI管理和用户交互
    void setupUi();
    void onButtonClicked();
};

class FrameSenderCore {
    // 只负责发送逻辑
    void startSending();
    void onTimerTick();
};

// ❌ 错误：一个类做太多事
class FrameSenderWidget {
    void setupUi();
    void onButtonClicked();
    void startSending();        // 应该在Core中
    void onTimerTick();         // 应该在Core中
    void loadConfig();          // 可以独立为ConfigManager
    void saveConfig();          // 可以独立为ConfigManager
};
```

#### 函数职责划分
```cpp
// ✅ 正确：函数职责单一
void setupUi() {
    createTitleBar();
    createToolbar();
    createTable();
    createButtonBar();
    createStatusBar();
}

void createTitleBar() {
    // 只创建标题栏
}

// ❌ 错误：函数做太多事
void setupUi() {
    // 500行代码创建所有UI...
}
```

### 11.9 代码审查检查清单

在提交代码前，请确保：

**文件大小**：
- [ ] 没有文件超过500行
- [ ] 超过300行的文件已计划拆分

**命名规范**：
- [ ] 类名使用大驼峰（PascalCase）
- [ ] 函数名使用小驼峰（camelCase）
- [ ] 成员变量使用m_前缀
- [ ] 槽函数使用on前缀

**注释规范**：
- [ ] 文件头注释完整（@file, @brief, @author, @date）
- [ ] 公共类有注释说明职责
- [ ] 公共函数有@brief和@param注释
- [ ] 复杂逻辑有行内注释

**代码质量**：
- [ ] 单一职责原则
- [ ] 错误处理完整（检查返回值、记录日志）
- [ ] 没有内存泄漏（使用父对象或智能指针）
- [ ] 性能优化合理（批量处理、避免频繁UI更新）

**UI规范**（参考ui-styles.md）：
- [ ] 遵循UI_STANDARDS.md
- [ ] 标题栏40px，颜色#607D8B
- [ ] 工具栏45px，颜色#E8EAF6
- [ ] 按钮栏60px，颜色#ECEFF1
- [ ] 状态栏30px，颜色#ECEFF1
- [ ] 按钮最小100x35px
- [ ] 使用标准按钮样式（主要/次要/危险）

**架构集成**：
- [ ] 使用ConnectionManager::sendFrame()发送
- [ ] 不破坏现有架构
- [ ] 遵循三层架构（表示层、业务层、数据层）

### 11.10 重构指南

#### 何时重构

**立即重构**：
- 文件超过500行
- 类职责不清晰
- 代码重复超过3次
- 函数超过50行

**计划重构**：
- 文件超过300行
- 函数超过30行
- 嵌套超过3层

#### 重构步骤

1. **分析职责**：识别类/函数的所有职责
2. **设计拆分**：规划如何拆分成多个类/函数
3. **创建新类**：先创建新的独立类
4. **迁移代码**：逐步迁移功能到新类
5. **更新调用**：更新所有调用点
6. **测试验证**：确保功能正常
7. **删除旧代码**：清理原有代码
8. **更新文档**：更新设计文档

#### 重构示例

**重构前**：
```cpp
// FrameSenderWidget.cpp (600行)
class FrameSenderWidget {
    void setupUi();           // 300行
    void loadConfig();        // 100行
    void saveConfig();        // 100行
    void onTimerTick();       // 100行
};
```

**重构后**：
```cpp
// FrameSenderWidget.cpp (300行)
class FrameSenderWidget {
    void setupUi();
    void onButtonClicked();
private:
    FrameSenderCore *m_core;
    FrameSenderConfig *m_configManager;
};

// FrameSenderCore.cpp (200行)
class FrameSenderCore {
    void startSending();
    void onTimerTick();
};

// FrameSenderConfig.cpp (200行)
class FrameSenderConfig {
    void loadConfig();
    void saveConfig();
};
```

---

## 12. 实施注意事项

### 12.1 开发顺序

1. **先创建数据结构**：FrameSendConfig、PlaybackState等
2. **再创建Core类**：FrameSenderCore、FramePlaybackCore
3. **最后创建Widget类**：FrameSenderWidget、FramePlaybackWidget
4. **逐步添加功能**：先MVP，后高级功能

### 12.2 测试策略

**边开发边测试**：
- 每完成一个函数，立即测试
- 每完成一个类，进行单元测试
- 每完成一个功能，进行集成测试

**使用qDebug调试**：
```cpp
qDebug() << "📊 当前配置数量:" << m_configs.size();
qDebug() << "⏱ 定时器间隔:" << elapsed << "ms";
qDebug() << "📡 发送帧:" << frame.frameId();
```

### 12.3 性能监控

**关键指标**：
- 发送速率：帧/秒
- 定时器精度：实际间隔 vs 期望间隔
- 内存使用：随时间变化
- CPU使用率：发送时的CPU占用

**监控代码示例**：
```cpp
// 统计发送速率
static int frameCount = 0;
static QElapsedTimer rateTimer;
if (!rateTimer.isValid()) {
    rateTimer.start();
}

frameCount++;
if (rateTimer.elapsed() >= 1000) {  // 每秒统计一次
    qDebug() << "📊 发送速率:" << frameCount << "帧/秒";
    frameCount = 0;
    rateTimer.restart();
}
```

### 12.4 常见问题预防

**问题1：定时器不精确**
- 解决：使用累积计数补偿误差
- 代码：见4.1节

**问题2：高速发送时UI卡顿**
- 解决：使用定时器批量刷新UI
- 代码：见11.6节

**问题3：内存泄漏**
- 解决：使用父对象或智能指针
- 代码：见11.7节

**问题4：文件超过500行**
- 解决：按职责拆分文件
- 代码：见11.1节

---

**文档版本**: v2.0 - 添加代码规范和架构约束  
**更新日期**: 2026-02-25  
**作者**: CANAnalyzerPro Team  
**状态**: 待审核
