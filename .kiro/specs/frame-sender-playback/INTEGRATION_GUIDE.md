# 菜单整合实施指南

## 概述

本指南说明如何将数据回放功能整合到现有的"发送数据"菜单中，替换原有的"单帧发送"功能，并将回放窗口嵌入到MDI区域（类似CANViewWidget）。

---

## 设计要点

1. **窗口类型**：嵌入MDI区域（使用CustomMdiSubWindow）
2. **菜单修改**：用"数据回放"替换"单帧发送"
3. **其他菜单项**：周期发送、发送列表、发送脚本暂时禁用（灰色）
4. **循环回放**：添加勾选框控制
5. **文件格式**：优先支持BLF和ASC
6. **参考实现**：CANViewWidget的创建方式

---

## 修改清单

### 1. MainWindow.h 修改

在 `MainWindow` 类中添加：

```cpp
// 在private slots:部分添加
private slots:
    void onDataPlayback();  // 新增：数据回放槽函数

// 在private:部分添加（窗口计数器）
private:
    int m_playbackWindowCount;    // 新增：回放窗口数量
    int m_playbackWindowNextId;   // 新增：回放窗口下一个编号
```

### 2. MainWindow.cpp 修改

#### 2.1 构造函数初始化

在 `MainWindow::MainWindow()` 构造函数中初始化：

```cpp
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_playbackWindowCount(0)     // 新增：初始化为0
    , m_playbackWindowNextId(1)    // 新增：从1开始编号
{
    // ... 其他初始化代码
}
```

#### 2.2 修改createToolbar()函数

找到"发送数据"菜单创建部分（约第260-273行），修改为：

```cpp
// 3. 发送数据
AnimatedToolButton *btnSend = new AnimatedToolButton();
btnSend->setText("发送数据");
btnSend->setIcon(QIcon(":/send.png"));
btnSend->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
btnSend->setIconSize(QSize(40, 40));
btnSend->setStyleSheet(toolBtnStyle);
btnSend->setPopupMode(QToolButton::InstantPopup);
QMenu *menuSend = new QMenu(btnSend);
menuSend->setStyleSheet(menuStyle);

// 修改：用"数据回放"替换"单帧发送"
menuSend->addAction("数据回放", this, &MainWindow::onDataPlayback);  // 新增

// 暂时禁用其他功能
QAction *actPeriodic = menuSend->addAction("周期发送");
actPeriodic->setEnabled(false);  // 禁用

QAction *actList = menuSend->addAction("发送列表");
actList->setEnabled(false);  // 禁用

QAction *actScript = menuSend->addAction("发送脚本");
actScript->setEnabled(false);  // 禁用

btnSend->setMenu(menuSend);
topLayout->addWidget(btnSend);
```

#### 2.3 实现onDataPlayback()槽函数（参考onNewCANView）

在 `MainWindow.cpp` 文件末尾添加：

```cpp
/**
 * @brief 创建数据回放窗口（嵌入MDI区域）
 * 
 * 参考：onNewCANView()的实现方式
 */
void MainWindow::onDataPlayback()
{
    qDebug() << "📡 创建数据回放窗口";
    
    // 检查窗口数量限制（可选，建议限制5个）
    if (m_playbackWindowCount >= 5) {
        QMessageBox::warning(this, QString::fromUtf8("提示"), 
            QString::fromUtf8("最多只能创建5个回放窗口\n"
                             "当前已有5个窗口，请关闭一些窗口后再试"));
        qDebug() << "⚠️ 已达到最大回放窗口数量限制（5个）";
        return;
    }
    
    // 🚀 创建FramePlaybackWidget（嵌入MDI）
    FramePlaybackWidget *playbackWidget = new FramePlaybackWidget();
    
    // 使用自定义MDI子窗口
    CustomMdiSubWindow *subWindow = new CustomMdiSubWindow();
    subWindow->setWidget(playbackWidget);
    
    // 🔧 使用编号计数器设置窗口标题（确保编号唯一）
    int windowId = m_playbackWindowNextId;
    subWindow->setWindowTitle(QString("回放%1").arg(windowId));
    
    // 🔧 增加窗口数量和编号
    m_playbackWindowCount++;      // 当前窗口数量+1
    m_playbackWindowNextId++;     // 下一个窗口编号+1（永远递增）
    
    // 添加到MDI区域
    m_mdiArea->addSubWindow(subWindow);
    subWindow->resize(900, 700);  // 设置初始大小
    subWindow->show();
    
    // 🔧 监听窗口销毁事件，只减少窗口数量（不减少编号）
    connect(subWindow, &QObject::destroyed, this, [this]() {
        m_playbackWindowCount--;  // 只减少数量
        qDebug() << "🗑 回放窗口已销毁，当前回放窗口数:" << m_playbackWindowCount;
        QTimer::singleShot(100, this, &MainWindow::arrangeSubWindows);
    });
    
    // 延迟自动排列窗口（避免频繁重排）
    m_resizeTimer->start(100);
}
```

### 3. 添加头文件引用

在 `MainWindow.cpp` 顶部添加：

```cpp
#include "FramePlaybackWidget.h"  // 新增
```

---

## 实施步骤

### 步骤1：修改MainWindow.h（5分钟）

1. 打开 `ZCANPRO/src/ui/MainWindow.h`
2. 在 `private slots:` 部分添加 `void onDataPlayback();`
3. 在 `private:` 部分添加：
   ```cpp
   int m_playbackWindowCount;
   int m_playbackWindowNextId;
   ```
4. 保存文件

### 步骤2：修改MainWindow.cpp（15分钟）

1. 打开 `ZCANPRO/src/ui/MainWindow.cpp`
2. 在文件顶部添加 `#include "FramePlaybackWidget.h"`
3. 在构造函数中初始化：
   ```cpp
   , m_playbackWindowCount(0)
   , m_playbackWindowNextId(1)
   ```
4. 找到"发送数据"菜单创建代码（约第270行）
5. 修改菜单项：
   - 添加"数据回放"并连接槽函数
   - 禁用"周期发送"、"发送列表"、"发送脚本"
6. 在文件末尾添加 `onDataPlayback()` 函数实现
7. 保存文件

### 步骤3：编译测试（5分钟）

1. 编译项目：`qmake && make`
2. 运行程序
3. 点击"发送数据"菜单
4. 验证菜单项：
   - ✅ 显示"数据回放"（不是"单帧发送"）
   - ✅ "周期发送"显示为灰色（禁用）
   - ✅ "发送列表"显示为灰色（禁用）
   - ✅ "发送脚本"显示为灰色（禁用）
5. 点击"数据回放"，验证窗口能在MDI区域打开
6. 验证窗口标题为"回放1"
7. 再次点击"数据回放"，验证新窗口标题为"回放2"

---

## 验收标准

- [ ] MainWindow.h 添加了 `onDataPlayback()` 声明
- [ ] MainWindow.h 添加了窗口计数器成员变量
- [ ] MainWindow.cpp 添加了 `FramePlaybackWidget.h` 引用
- [ ] MainWindow.cpp 构造函数初始化了计数器
- [ ] "发送数据"菜单显示"数据回放"而不是"单帧发送"
- [ ] "周期发送"、"发送列表"、"发送脚本"显示为灰色（禁用）
- [ ] 点击"数据回放"能在MDI区域打开窗口
- [ ] 窗口标题使用编号（回放1、回放2...）
- [ ] 窗口可以正常关闭和重新打开
- [ ] 窗口数量限制生效（最多5个）
- [ ] 编译无错误无警告

---

## 注意事项

1. **MDI窗口管理**：
   - 使用 `CustomMdiSubWindow` 包装内容widget
   - 使用 `m_mdiArea->addSubWindow()` 添加到MDI区域
   - 窗口编号永远递增，不重复使用

2. **窗口数量限制**：
   - 建议限制最多5个回放窗口（避免资源占用过多）
   - 可根据实际需求调整限制

3. **内存管理**：
   - CustomMdiSubWindow 会自动管理 FramePlaybackWidget 的生命周期
   - 窗口关闭时会触发 destroyed 信号，减少计数器

4. **窗口排列**：
   - 使用 `m_resizeTimer` 延迟触发自动排列
   - 避免频繁重排导致性能问题

5. **调试日志**：
   - 使用 `qDebug() << "📡 创建数据回放窗口";` 记录操作
   - 便于调试和追踪窗口生命周期

---

## 后续工作

完成菜单整合后，继续实施：

1. **阶段1**：创建 FramePlaybackWidget 基础框架
2. **阶段2**：实现 FramePlaybackCore 回放逻辑
3. **阶段3**：实现文件解析（BLF/ASC）
4. **阶段4**：优化和扩展功能

---

**文档版本**: v2.0  
**创建日期**: 2026-02-25  
**更新日期**: 2026-02-25  
**预计时间**: 0.5天  
**优先级**: P0（必须先完成）  
**参考实现**: MainWindow::onNewCANView()
