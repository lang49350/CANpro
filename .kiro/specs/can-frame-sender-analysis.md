# CAN帧发送功能分析与设计建议

## 1. 执行摘要

本文档分析了SavvyCAN的帧发送实现，并为ZCANPRO项目提供设计建议。

**关键发现**：
- SavvyCAN使用UI窗口+后台工作线程的双线程架构
- 支持多种触发模式：周期性、ID触发、信号触发
- 支持动态数据修改器（Modifiers）
- 使用1ms精确定时器进行帧发送

**建议方案**：
- 创建独立的FrameSenderWidget窗口（遵循ZCANPRO架构）
- 复用ConnectionManager进行帧发送
- 简化实现，先支持核心功能（周期发送、ID触发）
- 遵守500行文件大小限制

---

## 2. SavvyCAN架构分析

### 2.1 核心组件

**FrameSenderWindow** (UI层，1148行)
- 职责：用户界面、配置管理、文件保存/加载
- 表格列：Enable, Bus, ID, MsgName, Len, Ext, Rem, Data, Trigger, Modifications, Count
- 使用QTableWidget显示发送配置
- 1ms定时器驱动帧发送

**FrameSenderObject** (工作线程，~400行)
- 职责：后台发送线程、定时器管理
- 独立线程运行，避免阻塞UI
- 使用QMutex保护共享数据
- 高精度时间戳跟踪（微秒级）

**数据结构** (can_trigger_structs.h)
```cpp
// 触发器配置
class Trigger {
    bool readyCount;        // 是否准备好计数
    int ID;                 // 触发ID（-1=不关心）
    int milliseconds;       // 触发间隔（ms）
    int msCounter;          // 当前计数器
    int maxCount;           // 最大发送次数（-1=无限）
    int currCount;          // 当前已发送次数
    int bus;                // 总线号（-1=不关心）
    QString sigName;        // 信号名称（DBC）
    double sigValueDbl;     // 信号值触发条件
    uint32_t triggerMask;   // 触发掩码（组合条件）
};

// 修改器操作数
class ModifierOperand {
    int ID;                 // 0=常量, -1=影子寄存器, -2=自身数据, >0=外部帧ID
    int bus;                // 总线号
    int databyte;           // 数据字节索引或常量值
    bool notOper;           // 按位取反
    QString signalName;     // 信号名称（DBC）
};

// 修改器操作
class ModifierOp {
    ModifierOperand first;
    ModifierOperand second;
    ModifierOperationType operation;  // +, -, *, /, &, |, ^, %
};

// 修改器（一个数据字节的所有操作）
class Modifier {
    int destByte;           // 目标字节（-1=信号）
    QString signalName;     // 目标信号名称
    QList<ModifierOp> operations;  // 操作链
};

// 发送数据（继承自CANFrame）
class FrameSendData : public CANFrame {
    bool enabled;
    int count;
    QList<Trigger> triggers;
    QList<Modifier> modifiers;
};
```

### 2.2 触发模式详解

**1. 周期性触发 (Periodic)**
```
配置: "100MS"
含义: 每100ms发送一次
实现: msCounter累加，达到milliseconds时发送
```

**2. ID触发 (ID-based)**
```
配置: "ID0x200 5MS"
含义: 收到ID=0x200时，延迟5ms后发送
实现: processIncomingFrame()检查ID匹配，设置readyCount=true
```

**3. 次数限制 (Count-limited)**
```
配置: "100MS 50X"
含义: 每100ms发送一次，最多50次
实现: currCount++，达到maxCount时停止
```

**4. 总线过滤 (Bus-filtered)**
```
配置: "BUS0 100MS"
含义: 只在BUS0上发送
实现: 检查bus字段匹配
```

**5. 信号触发 (Signal-based, 需要DBC)**
```
配置: "ID0x200 SIG[BMS_Voltage;300]"
含义: 当ID=0x200的BMS_Voltage信号值=300时触发
实现: 使用DBC解析信号值，比较触发
```

**6. 组合触发**
```
配置: "ID0x200 5MS 10X BUS0, 100MS"
含义: 两个触发器
  - 触发器1: 收到BUS0上的ID=0x200时，延迟5ms发送，最多10次
  - 触发器2: 每100ms发送一次（无限）
```

### 2.3 修改器系统详解

修改器允许动态修改发送数据，支持复杂的数据计算。

**语法示例**：
```
D0=D0+1                    // 数据字节0自增1
D1=ID:0x200:D3+5           // D1 = 外部帧0x200的D3 + 5
D2=D0&0xF0                 // D2 = D0按位与0xF0
D3=ID:0x100:D2*2+D1        // D3 = (外部帧0x100的D2 * 2) + D1
```

**执行流程**：
1. 解析修改器字符串为操作链
2. 每次发送前执行doModifiers()
3. 使用影子寄存器累积中间结果
4. 最终结果写入目标字节

**操作数类型**：
- `0x12`: 常量
- `D0-D7`: 自身数据字节
- `ID:0x200:D3`: 外部帧的数据字节
- `~D0`: 按位取反

**支持的操作**：
- 算术: `+`, `-`, `*`, `/`, `%`
- 逻辑: `&`, `|`, `^`

### 2.4 定时器机制

**关键设计**：
```cpp
// 1ms精确定时器
intervalTimer->setTimerType(Qt::PreciseTimer);
intervalTimer->setInterval(1);

// 累积式计数（补偿定时器误差）
int elapsed = elapsedTimer.restart();
if (elapsed == 0) elapsed = 1;
trigger->msCounter += elapsed;

// 达到触发时间
if (trigger->msCounter >= trigger->milliseconds) {
    trigger->msCounter = 0;  // 重置计数器
    sendFrame();
}
```

**优点**：
- 即使定时器不精确（如2ms才触发一次），累积计数仍然准确
- 避免时间漂移

### 2.5 帧缓存机制

为了支持修改器从外部帧读取数据，维护了一个帧缓存：

```cpp
QMap<uint32_t, CANFrame> frameCache;  // ID -> 最新帧

// 收到新帧时更新缓存
void updatedFrames(int numFrames) {
    for (int i = ...) {
        CANFrame frame = modelFrames->at(i);
        frameCache[frame.frameId()] = frame;  // 覆盖旧帧
    }
}

// 修改器查找外部帧
CANFrame* lookupFrame(int ID, int bus) {
    if (!frameCache.contains(ID)) return nullptr;
    return &frameCache[ID];
}
```

---

## 3. ZCANPRO现有架构分析

### 3.1 三层架构

```
表示层: MainWindow, CANViewWidget
   ↓
业务层: ConnectionManager (单例), DataRouter (单例)
   ↓
数据层: SerialConnection, CANFrame
```

### 3.2 数据流向

**接收流程**：
```
ESP32 → SerialConnection → ConnectionManager::frameReceived
  → DataRouter::routeFrame → CANViewWidget::onFrameReceived
```

**发送流程（当前）**：
```
（尚未实现）
```

### 3.3 关键约束

1. **文件大小限制**: 单文件不超过500行
2. **单一职责原则**: 每个类只负责一个功能
3. **单例模式**: ConnectionManager和DataRouter是单例
4. **订阅模式**: 窗口通过DataRouter订阅设备+通道

---

## 4. 设计建议

### 4.1 整体架构

**推荐方案：简化的单线程架构**

```
┌─────────────────────────────────────────┐
│  FrameSenderWidget (UI + 发送逻辑)      │
│  - 配置表格                              │
│  - 1ms定时器                             │
│  - 触发器处理                            │
│  - 修改器处理                            │
└─────────────────────────────────────────┘
              ↓
┌─────────────────────────────────────────┐
│  ConnectionManager::sendFrame()         │
│  - 发送到指定设备                        │
└─────────────────────────────────────────┘
```

**理由**：
- ZCANPRO的数据量相对较小，单线程足够
- 简化实现，避免多线程复杂性
- 遵循现有架构模式
- 如果未来需要，可以重构为多线程

### 4.2 核心类设计

**FrameSenderWidget** (主窗口，预计400行)
```cpp
class FrameSenderWidget : public QWidget {
    Q_OBJECT
public:
    explicit FrameSenderWidget(QWidget *parent = nullptr);
    
    // 设备选择
    void setDevice(const QString &device);
    
    // 文件操作
    void loadConfig(const QString &filename);
    void saveConfig(const QString &filename);
    
private slots:
    void onTimerTick();           // 定时器触发
    void onCellChanged(int row, int col);
    void onEnableAll();
    void onDisableAll();
    void onClearAll();
    
private:
    void setupUi();
    void processTriggers();       // 处理所有触发器
    void sendFrame(int row);      // 发送指定行的帧
    
    QTableWidget *m_table;
    QTimer *m_timer;              // 1ms定时器
    QVector<FrameSendData> m_sendData;
    QString m_currentDevice;
};
```

**FrameSendData** (数据结构，预计100行)
```cpp
struct Trigger {
    int milliseconds;
    int msCounter;
    int maxCount;
    int currCount;
    bool enabled;
};

class FrameSendData {
public:
    bool enabled;
    QString device;
    int bus;
    uint32_t id;
    bool isExtended;
    QByteArray data;
    QList<Trigger> triggers;
    int sendCount;
};
```

### 4.3 分阶段实现计划

**阶段1：基础功能（MVP）**
- [ ] 创建FrameSenderWidget窗口
- [ ] 表格UI（Enable, Device, Bus, ID, Len, Data, Trigger, Count）
- [ ] 周期性触发（如"100MS"）
- [ ] 次数限制（如"100MS 50X"）
- [ ] 通过ConnectionManager发送
- [ ] 配置文件保存/加载

**预计代码量**：
- FrameSenderWidget.h: 80行
- FrameSenderWidget.cpp: 400行
- FrameSendData.h: 100行
- 总计: 580行 → 需要拆分

**阶段2：高级触发**
- [ ] ID触发（如"ID0x200 5MS"）
- [ ] 总线过滤（如"BUS0 100MS"）
- [ ] 组合触发（多个触发器）

**阶段3：修改器系统**
- [ ] 简单修改器（如"D0=D0+1"）
- [ ] 外部帧引用（如"D1=ID:0x200:D3"）
- [ ] 复杂表达式

**阶段4：DBC集成**
- [ ] 信号触发
- [ ] 信号修改器

### 4.4 文件拆分方案

为了遵守500行限制，建议拆分为：

**FrameSenderWidget.h** (80行)
- 类声明

**FrameSenderWidget.cpp** (300行)
- UI创建
- 基础槽函数
- 设备管理

**FrameSenderTrigger.cpp** (200行)
- 触发器解析
- 触发器处理
- 定时器逻辑

**FrameSenderModifier.cpp** (200行)
- 修改器解析
- 修改器执行

**FrameSenderFile.cpp** (150行)
- 配置文件保存
- 配置文件加载

**FrameSendData.h** (100行)
- 数据结构定义

### 4.5 UI设计

**表格列**：
```
[✓] | Device | Bus | ID   | Len | Ext | Data              | Trigger    | Count
----+--------+-----+------+-----+-----+-------------------+------------+------
[✓] | COM3   | 0   | 0x123| 8   | [ ] | 01 02 03 04 ...   | 100MS      | 0
[ ] | COM3   | 1   | 0x456| 8   | [✓] | AA BB CC DD ...   | ID0x200 5MS| 0
```

**按钮栏**：
```
[设备: COM3 ▼] [启用全部] [禁用全部] [清空] [加载配置] [保存配置]
```

**状态栏**：
```
已发送: 1234 帧 | 定时器: 运行中
```

### 4.6 关键代码示例

**定时器处理**：
```cpp
void FrameSenderWidget::onTimerTick() {
    int elapsed = m_elapsedTimer.restart();
    if (elapsed == 0) elapsed = 1;
    
    for (int i = 0; i < m_sendData.size(); i++) {
        FrameSendData &data = m_sendData[i];
        if (!data.enabled) continue;
        
        for (Trigger &trigger : data.triggers) {
            if (!trigger.enabled) continue;
            if (trigger.currCount >= trigger.maxCount && trigger.maxCount > 0)
                continue;
            
            trigger.msCounter += elapsed;
            if (trigger.msCounter >= trigger.milliseconds) {
                trigger.msCounter = 0;
                trigger.currCount++;
                data.sendCount++;
                
                // 发送帧
                CANFrame frame;
                frame.setFrameId(data.id);
                frame.setExtendedFrameFormat(data.isExtended);
                frame.setPayload(data.data);
                frame.bus = data.bus;
                
                ConnectionManager::instance()->sendFrame(data.device, frame);
                
                // 更新UI
                updateRow(i);
            }
        }
    }
}
```

**触发器解析**：
```cpp
QList<Trigger> FrameSenderWidget::parseTriggers(const QString &text) {
    // "100MS, ID0x200 5MS 10X"
    QList<Trigger> triggers;
    QStringList parts = text.split(',');
    
    for (const QString &part : parts) {
        Trigger trigger;
        trigger.milliseconds = 100;  // 默认
        trigger.maxCount = -1;       // 无限
        trigger.msCounter = 0;
        trigger.currCount = 0;
        trigger.enabled = true;
        
        QStringList tokens = part.trimmed().split(' ');
        for (const QString &token : tokens) {
            if (token.endsWith("MS")) {
                trigger.milliseconds = token.left(token.length()-2).toInt();
            }
            else if (token.endsWith("X")) {
                trigger.maxCount = token.left(token.length()-1).toInt();
            }
        }
        
        triggers.append(trigger);
    }
    
    return triggers;
}
```

---

## 5. 与SavvyCAN的差异

| 特性 | SavvyCAN | ZCANPRO建议 |
|------|----------|-------------|
| 线程模型 | 双线程（UI+工作线程） | 单线程（简化） |
| 修改器 | 完整支持 | 阶段3实现 |
| DBC集成 | 完整支持 | 阶段4实现 |
| 信号触发 | 支持 | 阶段4实现 |
| 文件格式 | .fsd文本格式 | JSON格式（更易解析） |
| 帧缓存 | QMap<ID, Frame> | 复用DataRouter的数据 |

---

## 6. 风险与挑战

### 6.1 技术风险

**定时器精度**
- 风险: Windows定时器精度可能不足1ms
- 缓解: 使用累积计数补偿误差

**高速发送**
- 风险: 单线程可能无法支持极高速率（>5000帧/秒）
- 缓解: 先实现单线程，性能不足时再重构为多线程

**文件大小限制**
- 风险: 功能复杂可能导致文件超过500行
- 缓解: 提前规划文件拆分

### 6.2 用户体验风险

**配置复杂性**
- 风险: 触发器语法对新用户不友好
- 缓解: 提供可视化配置对话框（类似SavvyCAN的TriggerDialog）

**错误处理**
- 风险: 配置错误导致发送失败
- 缓解: 实时验证配置，显示错误提示

---

## 7. 实施建议

### 7.1 开发顺序

1. **创建基础UI**（1-2天）
   - FrameSenderWidget窗口
   - 表格布局
   - 按钮功能

2. **实现周期发送**（2-3天）
   - 定时器机制
   - 简单触发器解析
   - 通过ConnectionManager发送

3. **配置文件**（1天）
   - JSON格式保存/加载

4. **测试与优化**（1-2天）
   - 测试不同发送速率
   - 优化性能

5. **高级功能**（按需）
   - ID触发
   - 修改器
   - DBC集成

### 7.2 测试策略

**单元测试**：
- 触发器解析正确性
- 定时器计数准确性
- 修改器计算正确性

**集成测试**：
- 与ConnectionManager集成
- 多设备发送
- 高速发送稳定性

**性能测试**：
- 1000帧/秒持续发送
- 多行同时发送
- 长时间运行稳定性

---

## 8. 参考资料

**SavvyCAN源码**：
- `framesenderwindow.h/cpp`: UI和主逻辑
- `framesenderobject.h/cpp`: 工作线程
- `can_trigger_structs.h`: 数据结构
- `triggerdialog.h/cpp`: 可视化配置对话框

**ZCANPRO架构**：
- `Architecture.md`: 架构设计文档
- `ConnectionManager.h`: 连接管理器
- `DataRouter.h`: 数据路由器
- `CANViewWidget.h`: 视图窗口示例

---

## 9. 总结

**核心建议**：
1. 采用简化的单线程架构
2. 分阶段实现，先MVP后高级功能
3. 严格遵守500行文件限制
4. 复用现有ConnectionManager
5. 参考SavvyCAN的触发器和修改器设计

**预期成果**：
- 功能完整的帧发送工具
- 代码结构清晰，易于维护
- 性能满足常规使用场景（<5000帧/秒）
- 为未来扩展（DBC、信号）预留接口

**下一步行动**：
1. 审查本文档，确认设计方案
2. 创建FrameSenderWidget基础框架
3. 实现MVP功能
4. 迭代优化

---

**文档版本**: v1.0  
**创建日期**: 2026-02-25  
**作者**: CANAnalyzerPro Team


---

## 10. ZLG ZCANPRO功能分析（补充）

基于ZLG官方文档和配置文件分析，ZCANPRO提供了以下核心功能：

### 10.1 回放功能（Playback）

**核心特性**：
1. **在线回放**：通过CAN设备实际发送数据到总线
2. **离线回放**：仅在界面显示，不发送到总线
3. **回放模式**：
   - 正常回放：按文件时间戳完整回放
   - 步进回放：按时间间隔分段回放，方便观察
   - 暂停/继续：可以暂停并从当前位置继续
   - 取消：重新开始回放

**倍速控制**：
- 尽可能快：忽略时间间隔，全速回放
- 按倍率：可设置倍率（默认1.0）

**通道映射**：
- 将文件中的任意通道映射到目标通道
- 支持多通道文件回放到单通道设备

**过滤功能**：
- 白名单/黑名单模式
- 按通道、类型、帧类型、方向、ID范围过滤

**配置示例**（playback.json）：
```json
{
    "file_name_list": "",
    "filter": {
        "frmCanType": 3,      // CAN类型过滤
        "frmData": "",        // 数据过滤
        "frmDir": 3,          // 方向过滤
        "frmDlc": "",         // 长度过滤
        "frmFormat": 3,       // 格式过滤
        "frmIds": "",         // ID过滤
        "frmType": 3          // 帧类型过滤
    },
    "queue_send_enable": "true",  // 队列发送
    "speed": "1",                 // 倍速
    "step": "10",                 // 步进间隔
    "step_enable": "false",       // 步进使能
    "times": "1"                  // 回放次数
}
```

### 10.2 简单发送功能（Simple Transmit）

**核心特性**：
1. **基础配置**：
   - 帧ID、数据长度、数据内容
   - 标准帧/扩展帧
   - 数据帧/远程帧

2. **发送模式**：
   - 单次发送
   - 周期发送（可设置间隔）
   - 重复次数控制

3. **高级功能**：
   - ID自增：每次发送ID递增
   - 数据自增：指定字节自增
   - 位操作：支持位掩码和位运算
   - 列表发送：批量发送多个帧

4. **发送速率**：
   - 预设速率选项（transmit_rate_selected: 6）
   - 自定义间隔时间

**配置示例**（simple transmit.json）：
```json
{
    "tab0": {
        "frame id": "100",
        "data len selected": 8,
        "datas": "00 11 22 33 44 55 66 77",
        "frame format value": 0,      // 0=标准帧, 1=扩展帧
        "frame type value": 0,        // 0=数据帧, 1=远程帧
        "transmit mode value": 0,     // 发送模式
        "interval": "0",              // 间隔时间
        "transmit time": "1",         // 发送次数
        "frame repeat": "1",          // 重复次数
        
        // 高级功能
        "increase id": false,         // ID自增
        "increase data": false,       // 数据自增
        "increase data start byte": 0,
        "increase data length": 8,
        
        // 位操作
        "bit_operator_enable": false,
        "bit mask": "ffffffff",
        "bit operator": 0,
        
        // 列表发送
        "list advance": false,
        "list send mode": 0,
        "list transmit interval": 0,
        "list transmit time": "1"
    }
}
```

### 10.3 DBC发送功能（DBC Transmit）

**核心特性**：
1. **DBC集成**：
   - 加载DBC文件
   - 通道关联DBC
   - 从数据库添加报文

2. **信号编辑**：
   - 可视化编辑信号值
   - 自动计算数据字节
   - 实时预览

3. **协议支持**：
   - CAN 2.0
   - CAN-FD
   - J1939（支持SA/DA配置）

4. **发送控制**：
   - 周期发送
   - 发送速率控制
   - 失败处理模式

**配置示例**（dbc_transmit3.json）：
```json
{
    "tab0": {
        "dbcFile": "",
        "protocol": 0,            // 0=CAN, 1=CAN-FD, 2=J1939
        "j1939SA": "00",          // J1939源地址
        "j1939DA": "FF",          // J1939目标地址
        "sendSpeed": 6,           // 发送速率
        "transFailMode": 0,       // 失败处理模式
        "msgList": []             // 消息列表
    }
}
```

### 10.4 ZLG vs SavvyCAN 功能对比

| 功能 | ZLG ZCANPRO | SavvyCAN | ZCANPRO建议 |
|------|-------------|----------|-------------|
| **回放功能** | ✅ 完整支持 | ✅ 支持 | 优先实现 |
| 倍速回放 | ✅ 支持 | ✅ 支持 | 阶段1 |
| 步进回放 | ✅ 支持 | ❌ 不支持 | 阶段2（特色功能） |
| 通道映射 | ✅ 支持 | ❌ 不支持 | 阶段2 |
| 过滤回放 | ✅ 支持 | ✅ 支持 | 阶段1 |
| **简单发送** | ✅ 完整支持 | ✅ 支持 | 优先实现 |
| 周期发送 | ✅ 支持 | ✅ 支持 | 阶段1 |
| ID自增 | ✅ 支持 | ❌ 不支持 | 阶段2 |
| 数据自增 | ✅ 支持 | ✅ 支持（修改器） | 阶段2 |
| 位操作 | ✅ 支持 | ✅ 支持（修改器） | 阶段3 |
| 列表发送 | ✅ 支持 | ✅ 支持 | 阶段1 |
| **DBC发送** | ✅ 完整支持 | ✅ 支持 | 阶段4 |
| 信号编辑 | ✅ 可视化 | ✅ 可视化 | 阶段4 |
| J1939支持 | ✅ 支持 | ✅ 支持 | 阶段4 |

### 10.5 ZCANPRO设计亮点

**1. 步进回放**（独特功能）
- 按时间间隔分段回放
- 方便观察和调试
- 建议实现：每次步进N毫秒，暂停等待用户操作

**2. 通道映射**（实用功能）
- 解决单通道设备回放多通道文件的问题
- 灵活的通道路由
- 建议实现：简单的下拉选择映射

**3. ID/数据自增**（便捷功能）
- 快速生成测试数据
- 压力测试场景
- 建议实现：勾选框+起始值+步长

**4. 列表发送**（批量功能）
- 一次配置多个帧
- 批量发送
- 建议实现：表格形式，支持导入/导出

### 10.6 更新后的实施建议

**阶段1：基础发送（MVP）** - 2-3天
- [x] 周期发送（参考ZLG的interval配置）
- [x] 次数限制（参考ZLG的transmit_time）
- [x] 列表发送（参考ZLG的list模式）
- [x] 配置保存/加载（JSON格式）

**阶段2：高级发送** - 2-3天
- [ ] ID自增（参考ZLG的increase_id）
- [ ] 数据自增（参考ZLG的increase_data）
- [ ] 步进回放（ZLG特色功能）
- [ ] 通道映射（ZLG特色功能）

**阶段3：回放功能** - 3-4天
- [ ] 文件回放（BLF/ASC/CSV）
- [ ] 倍速控制（参考ZLG的speed）
- [ ] 过滤回放（参考ZLG的filter）
- [ ] 暂停/继续/取消

**阶段4：DBC集成** - 按需
- [ ] DBC文件加载
- [ ] 信号编辑界面
- [ ] 自动计算数据

### 10.7 UI设计建议（参考ZLG）

**发送窗口布局**：
```
┌─────────────────────────────────────────────────────────┐
│ [设备: COM3 ▼] [通道: CAN0 ▼]                           │
├─────────────────────────────────────────────────────────┤
│ [✓] | ID    | Len | Ext | Data              | 间隔 | 次数│
│ [✓] | 0x100 | 8   | [ ] | 00 11 22 33 ...   | 100ms| 10 │
│ [ ] | 0x200 | 8   | [✓] | AA BB CC DD ...   | 50ms | ∞  │
│ [✓] | 0x300 | 4   | [ ] | 01 02 03 04       | 200ms| 1  │
├─────────────────────────────────────────────────────────┤
│ [启动全部] [停止全部] [清空] [加载] [保存]              │
│                                                          │
│ 高级选项:                                                │
│ [✓] ID自增  起始:0x100  步长:1                          │
│ [✓] 数据自增 字节:0  起始:0  步长:1                     │
└─────────────────────────────────────────────────────────┘
```

**回放窗口布局**：
```
┌─────────────────────────────────────────────────────────┐
│ 文件: [选择文件...] [test.blf]                          │
│                                                          │
│ 回放模式: ⦿ 正常  ○ 步进(10ms)  ○ 尽可能快             │
│ 倍速: [1.0x ▼]  次数: [1 ▼]                            │
│                                                          │
│ 通道映射:                                                │
│ 文件CAN1 → [CAN0 ▼]                                     │
│ 文件CAN2 → [CAN0 ▼]                                     │
│                                                          │
│ 过滤: [✓] 启用  模式: [白名单 ▼]                        │
│ ID范围: [0x100] - [0x200]                               │
│                                                          │
│ [▶ 开始] [⏸ 暂停] [⏹ 停止] [⏭ 步进]                    │
│ ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ │
│ 进度: 45% | 已发送: 1234/2700 帧 | 用时: 00:05          │
└─────────────────────────────────────────────────────────┘
```

---

## 11. 综合建议（更新）

结合SavvyCAN和ZLG ZCANPRO的分析，为ZCANPRO项目提供以下建议：

### 11.1 功能优先级

**P0（必须实现）**：
1. 周期发送（参考ZLG的简单发送）
2. 列表发送（多帧配置）
3. 配置保存/加载（JSON格式）
4. 基础UI（表格+按钮）

**P1（重要功能）**：
1. ID/数据自增（ZLG特色）
2. 次数限制
3. 文件回放（BLF/ASC）
4. 倍速控制

**P2（增强功能）**：
1. 步进回放（ZLG独有，建议实现）
2. 通道映射（ZLG独有）
3. 过滤回放
4. 触发发送（SavvyCAN特色）

**P3（高级功能）**：
1. 修改器系统（SavvyCAN特色）
2. DBC集成
3. 信号编辑

### 11.2 架构建议（最终版）

**推荐方案：混合架构**

```
┌─────────────────────────────────────────────────────────┐
│  FrameSenderWidget (UI + 简单发送)                      │
│  - 周期发送                                              │
│  - ID/数据自增                                           │
│  - 列表发送                                              │
└─────────────────────────────────────────────────────────┘
              ↓
┌─────────────────────────────────────────────────────────┐
│  FramePlaybackWidget (回放功能)                         │
│  - 文件解析                                              │
│  - 倍速控制                                              │
│  - 步进回放                                              │
│  - 通道映射                                              │
└─────────────────────────────────────────────────────────┘
              ↓
┌─────────────────────────────────────────────────────────┐
│  ConnectionManager::sendFrame()                         │
└─────────────────────────────────────────────────────────┘
```

**理由**：
- 发送和回放功能分离，符合单一职责原则
- 每个窗口独立，易于维护
- 复用ConnectionManager，统一发送接口

### 11.3 实施路线图（最终版）

**第1周：基础发送**
- Day 1-2: FrameSenderWidget UI + 周期发送
- Day 3-4: ID/数据自增 + 列表发送
- Day 5: 配置保存/加载 + 测试

**第2周：回放功能**
- Day 1-2: FramePlaybackWidget UI + 文件解析
- Day 3-4: 倍速控制 + 步进回放
- Day 5: 通道映射 + 过滤 + 测试

**第3周：优化与扩展**
- Day 1-2: 性能优化 + Bug修复
- Day 3-4: 触发发送（可选）
- Day 5: 文档编写 + 用户测试

### 11.4 关键代码示例（更新）

**ID自增实现**（参考ZLG）：
```cpp
void FrameSenderWidget::onTimerTick() {
    for (FrameSendData &data : m_sendData) {
        if (!data.enabled) continue;
        
        // 发送帧
        CANFrame frame;
        frame.setFrameId(data.currentId);
        frame.setPayload(data.data);
        
        ConnectionManager::instance()->sendFrame(data.device, frame);
        
        // ID自增
        if (data.idIncreaseEnabled) {
            data.currentId += data.idIncreaseStep;
            if (data.currentId > data.idIncreaseMax) {
                data.currentId = data.idIncreaseStart;
            }
        }
        
        // 数据自增
        if (data.dataIncreaseEnabled) {
            int byteIndex = data.dataIncreaseByteIndex;
            data.data[byteIndex] += data.dataIncreaseStep;
        }
    }
}
```

**步进回放实现**（参考ZLG）：
```cpp
void FramePlaybackWidget::onStepForward() {
    if (!m_isPlaying) return;
    
    // 计算步进时间窗口
    quint64 stepEndTime = m_currentTime + m_stepInterval;
    
    // 发送时间窗口内的所有帧
    while (m_currentFrameIndex < m_frames.size()) {
        const CANFrame &frame = m_frames[m_currentFrameIndex];
        if (frame.timestamp > stepEndTime) break;
        
        // 应用通道映射
        int targetChannel = getChannelMapping(frame.channel);
        
        // 应用过滤
        if (!passFilter(frame)) {
            m_currentFrameIndex++;
            continue;
        }
        
        // 发送帧
        CANFrame sendFrame = frame;
        sendFrame.channel = targetChannel;
        ConnectionManager::instance()->sendFrame(m_device, sendFrame);
        
        m_currentFrameIndex++;
    }
    
    m_currentTime = stepEndTime;
    updateProgress();
    
    // 自动暂停，等待用户点击下一步
    m_isPlaying = false;
}
```

---

**文档版本**: v2.0 - 加入ZLG ZCANPRO功能分析  
**更新日期**: 2026-02-25  
**作者**: CANAnalyzerPro Team
