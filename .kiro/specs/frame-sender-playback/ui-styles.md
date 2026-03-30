# 帧发送与回放功能UI样式规范

本文档严格遵循 `ZCANPRO/docs/UI_STANDARDS.md` 的设计规范。

---

## 1. FrameSenderWidget（帧发送窗口）

### 1.1 整体布局

```
┌──────────────────────────────────────────────────────────────┐
│ 标题栏 (40px, #607D8B)                                 [×]   │
├──────────────────────────────────────────────────────────────┤
│ 工具栏 (45px, #E8EAF6)                                       │
│ 设备: [COM3 ▼]  通道: [CAN0 ▼]                              │
├──────────────────────────────────────────────────────────────┤
│                                                              │
│ 表格区域 (白色背景)                                          │
│ [✓] | ID    | Len | Ext | Data          | 间隔  | 次数     │
│ [✓] | 0x100 | 8   | [ ] | 00 11 22 ...  | 100ms | 10      │
│                                                              │
│ [+ 添加行]                                                   │
│                                                              │
│ 高级选项 (可折叠, #F5F5F5)                                   │
│ [✓] ID自增   起始:0x100  步长:1  最大:0x1FF                 │
│                                                              │
├──────────────────────────────────────────────────────────────┤
│ 按钮栏 (60px, #ECEFF1)                                       │
│                    [启动全部] [停止全部] [清空] [加载] [保存]│
├──────────────────────────────────────────────────────────────┤
│ 状态栏 (30px, #ECEFF1)                                       │
│ 已发送: 1234 帧 | 运行中                                     │
└──────────────────────────────────────────────────────────────┘
```

### 1.2 代码实现

#### 标题栏（严格遵循规范）
```cpp
QWidget *titleBar = new QWidget();
titleBar->setMinimumHeight(40);
titleBar->setMaximumHeight(40);
titleBar->setStyleSheet("QWidget { background-color: #607D8B; }");

QHBoxLayout *titleLayout = new QHBoxLayout(titleBar);
titleLayout->setContentsMargins(15, 0, 5, 0);

QLabel *lblTitle = new QLabel("帧发送");
lblTitle->setStyleSheet(
    "color: white; "
    "font-size: 14px; "
    "font-weight: bold; "
    "font-family: 'Microsoft YaHei UI';"
);
titleLayout->addWidget(lblTitle);
titleLayout->addStretch();

// 关闭按钮
QPushButton *btnClose = new QPushButton("✕");
btnClose->setStyleSheet(
    "QPushButton { "
    "   background-color: transparent; "
    "   color: white; "
    "   border: none; "
    "   min-width: 40px; max-width: 40px; "
    "   min-height: 40px; max-height: 40px; "
    "   font-size: 16px; "
    "} "
    "QPushButton:hover { "
    "   background-color: #E81123; "
    "}"
);
titleLayout->addWidget(btnClose);
```

#### 工具栏
```cpp
QWidget *toolbar = new QWidget();
toolbar->setMinimumHeight(45);
toolbar->setMaximumHeight(45);
toolbar->setStyleSheet("QWidget { background-color: #E8EAF6; }");

QHBoxLayout *toolbarLayout = new QHBoxLayout(toolbar);
toolbarLayout->setContentsMargins(15, 5, 15, 5);
toolbarLayout->setSpacing(10);

// 设备标签
QLabel *lblDevice = new QLabel("设备:");
lblDevice->setStyleSheet("font-size: 12px; font-family: 'Microsoft YaHei UI';");
toolbarLayout->addWidget(lblDevice);

// 设备下拉框
QComboBox *cmbDevice = new QComboBox();
cmbDevice->setMinimumWidth(120);
cmbDevice->setStyleSheet(
    "QComboBox { "
    "   background-color: white; "
    "   border: 1px solid #A4B5C8; "
    "   border-radius: 3px; "
    "   padding: 5px 10px; "
    "   font-size: 12px; "
    "   font-family: 'Microsoft YaHei UI'; "
    "} "
    "QComboBox:hover { "
    "   border: 1px solid #64B5F6; "
    "} "
    "QComboBox::drop-down { "
    "   border: none; "
    "   width: 20px; "
    "}"
);
toolbarLayout->addWidget(cmbDevice);

// 通道标签
QLabel *lblChannel = new QLabel("通道:");
lblChannel->setStyleSheet("font-size: 12px; font-family: 'Microsoft YaHei UI';");
toolbarLayout->addWidget(lblChannel);

// 通道下拉框
QComboBox *cmbChannel = new QComboBox();
cmbChannel->setMinimumWidth(100);
cmbChannel->setStyleSheet(/* 同设备下拉框 */);
toolbarLayout->addWidget(cmbChannel);

toolbarLayout->addStretch();
```

#### 表格样式
```cpp
QTableWidget *table = new QTableWidget();
table->setStyleSheet(
    "QTableWidget { "
    "   background-color: white; "
    "   border: 1px solid #E0E0E0; "
    "   gridline-color: #E0E0E0; "
    "   font-size: 12px; "
    "   font-family: 'Microsoft YaHei UI'; "
    "} "
    "QTableWidget::item { "
    "   padding: 5px; "
    "   border: none; "
    "} "
    "QTableWidget::item:selected { "
    "   background-color: #E3F2FD; "
    "   color: black; "
    "} "
    "QHeaderView::section { "
    "   background-color: #F5F5F5; "
    "   color: black; "
    "   padding: 8px; "
    "   border: 1px solid #E0E0E0; "
    "   font-size: 12px; "
    "   font-weight: bold; "
    "   font-family: 'Microsoft YaHei UI'; "
    "}"
);
```

#### 按钮栏（严格遵循规范）
```cpp
QWidget *buttonBar = new QWidget();
buttonBar->setStyleSheet("QWidget { background-color: #ECEFF1; }");
buttonBar->setMinimumHeight(60);
buttonBar->setMaximumHeight(60);

QHBoxLayout *buttonLayout = new QHBoxLayout(buttonBar);
buttonLayout->setContentsMargins(20, 10, 20, 10);
buttonLayout->addStretch();

// 启动全部按钮（主要按钮）
QPushButton *btnStartAll = new QPushButton("启动全部");
btnStartAll->setMinimumWidth(100);
btnStartAll->setMinimumHeight(35);
btnStartAll->setStyleSheet(
    "QPushButton { "
    "   background-color: #E3F2FD; "
    "   color: black; "
    "   border: 1px solid #90CAF9; "
    "   padding: 8px 30px; "
    "   font-size: 12px; "
    "   font-family: 'Microsoft YaHei UI'; "
    "   border-radius: 3px; "
    "} "
    "QPushButton:hover { "
    "   background-color: #BBDEFB; "
    "   border: 1px solid #64B5F6; "
    "} "
    "QPushButton:pressed { "
    "   background-color: #90CAF9; "
    "}"
);
buttonLayout->addWidget(btnStartAll);

// 停止全部按钮（危险按钮）
QPushButton *btnStopAll = new QPushButton("停止全部");
btnStopAll->setMinimumWidth(100);
btnStopAll->setMinimumHeight(35);
btnStopAll->setStyleSheet(
    "QPushButton { "
    "   background-color: #FFEBEE; "
    "   color: #C62828; "
    "   border: 1px solid #EF9A9A; "
    "   padding: 8px 30px; "
    "   font-size: 12px; "
    "   font-family: 'Microsoft YaHei UI'; "
    "   border-radius: 3px; "
    "} "
    "QPushButton:hover { "
    "   background-color: #FFCDD2; "
    "   border: 1px solid #E57373; "
    "} "
    "QPushButton:pressed { "
    "   background-color: #EF9A9A; "
    "}"
);
buttonLayout->addWidget(btnStopAll);

// 清空按钮（次要按钮）
QPushButton *btnClear = new QPushButton("清空");
btnClear->setMinimumWidth(100);
btnClear->setMinimumHeight(35);
btnClear->setStyleSheet(
    "QPushButton { "
    "   background-color: #F5F5F5; "
    "   color: black; "
    "   border: 1px solid #CCCCCC; "
    "   padding: 8px 30px; "
    "   font-size: 12px; "
    "   font-family: 'Microsoft YaHei UI'; "
    "   border-radius: 3px; "
    "} "
    "QPushButton:hover { "
    "   background-color: #E0E0E0; "
    "   border: 1px solid #999999; "
    "} "
    "QPushButton:pressed { "
    "   background-color: #CCCCCC; "
    "}"
);
buttonLayout->addWidget(btnClear);

// 加载配置按钮（次要按钮）
QPushButton *btnLoad = new QPushButton("加载配置");
btnLoad->setMinimumWidth(100);
btnLoad->setMinimumHeight(35);
btnLoad->setStyleSheet(/* 同清空按钮 */);
buttonLayout->addWidget(btnLoad);

// 保存配置按钮（次要按钮）
QPushButton *btnSave = new QPushButton("保存配置");
btnSave->setMinimumWidth(100);
btnSave->setMinimumHeight(35);
btnSave->setStyleSheet(/* 同清空按钮 */);
buttonLayout->addWidget(btnSave);
```

#### 状态栏（严格遵循规范）
```cpp
QWidget *statusBar = new QWidget();
statusBar->setMinimumHeight(30);
statusBar->setMaximumHeight(30);
statusBar->setStyleSheet("QWidget { background-color: #ECEFF1; }");

QHBoxLayout *statusLayout = new QHBoxLayout(statusBar);
statusLayout->setContentsMargins(10, 0, 10, 0);

QLabel *lblStatus = new QLabel("已发送: 0 帧 | 就绪");
lblStatus->setStyleSheet(
    "color: black; "
    "font-size: 12px; "
    "font-family: 'Microsoft YaHei UI';"
);
statusLayout->addWidget(lblStatus);
statusLayout->addStretch();
```

---

## 2. FramePlaybackWidget（回放窗口）

### 2.1 整体布局

```
┌──────────────────────────────────────────────────────────────┐
│ 标题栏 (40px, #607D8B)                                 [×]   │
├──────────────────────────────────────────────────────────────┤
│ 工具栏 (45px, #E8EAF6)                                       │
│ 设备: [COM3 ▼]  通道: [CAN0 ▼]                              │
├──────────────────────────────────────────────────────────────┤
│                                                              │
│ 序列列表 (白色背景)                                          │
│ 文件名                          | 循环次数                    │
│ test_data.blf                   | 1                         │
│                                                              │
│ [加载文件] [删除]                                            │
│                                                              │
│ 回放控制 (白色背景)                                          │
│ [⏮] [⏸] [◀] [⏹] [▶] [⏭]                                   │
│ ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ │
│ 45% | 1234/2700 帧 | 用时: 00:05                           │
│                                                              │
│ 回放设置 (白色背景)                                          │
│ 模式: ⦿ 正常  ○ 步进  ○ 尽可能快                           │
│ 倍速: [1.0x ▼]  步进间隔: [10ms]                            │
│ [✓] 循环回放整个序列                                         │
│                                                              │
│ ID过滤 (白色背景)                                            │
│ [✓] 0x100  [✓] 0x200  [ ] 0x300                            │
│                                                              │
├──────────────────────────────────────────────────────────────┤
│ 按钮栏 (60px, #ECEFF1)                                       │
│                                        [全选] [全不选] [确定]│
├──────────────────────────────────────────────────────────────┤
│ 状态栏 (30px, #ECEFF1)                                       │
│ 当前文件: test_data.blf | 正在播放                          │
└──────────────────────────────────────────────────────────────┘
```

### 2.2 代码实现

#### 标题栏（同发送窗口）
```cpp
// 完全相同的实现，只修改标题文字
QLabel *lblTitle = new QLabel("帧回放");
```

#### 工具栏（同发送窗口）
```cpp
// 完全相同的实现
```

#### 序列列表
```cpp
QListWidget *sequenceList = new QListWidget();
sequenceList->setStyleSheet(
    "QListWidget { "
    "   background-color: white; "
    "   border: 1px solid #E0E0E0; "
    "   font-size: 12px; "
    "   font-family: 'Microsoft YaHei UI'; "
    "} "
    "QListWidget::item { "
    "   padding: 8px; "
    "   border-bottom: 1px solid #F5F5F5; "
    "} "
    "QListWidget::item:hover { "
    "   background-color: #F5F5F5; "
    "} "
    "QListWidget::item:selected { "
    "   background-color: #E3F2FD; "
    "   color: black; "
    "}"
);
```

#### 回放控制按钮（使用标准按钮样式）
```cpp
// 播放按钮（主要按钮）
QPushButton *btnPlay = new QPushButton("▶ 播放");
btnPlay->setMinimumWidth(100);
btnPlay->setMinimumHeight(35);
btnPlay->setStyleSheet(
    "QPushButton { "
    "   background-color: #E3F2FD; "
    "   color: black; "
    "   border: 1px solid #90CAF9; "
    "   padding: 8px 30px; "
    "   font-size: 12px; "
    "   font-family: 'Microsoft YaHei UI'; "
    "   border-radius: 3px; "
    "} "
    "QPushButton:hover { "
    "   background-color: #BBDEFB; "
    "   border: 1px solid #64B5F6; "
    "} "
    "QPushButton:pressed { "
    "   background-color: #90CAF9; "
    "}"
);

// 暂停按钮（次要按钮）
QPushButton *btnPause = new QPushButton("⏸ 暂停");
btnPause->setMinimumWidth(100);
btnPause->setMinimumHeight(35);
btnPause->setStyleSheet(
    "QPushButton { "
    "   background-color: #F5F5F5; "
    "   color: black; "
    "   border: 1px solid #CCCCCC; "
    "   padding: 8px 30px; "
    "   font-size: 12px; "
    "   font-family: 'Microsoft YaHei UI'; "
    "   border-radius: 3px; "
    "} "
    "QPushButton:hover { "
    "   background-color: #E0E0E0; "
    "   border: 1px solid #999999; "
    "} "
    "QPushButton:pressed { "
    "   background-color: #CCCCCC; "
    "}"
);

// 停止按钮（危险按钮）
QPushButton *btnStop = new QPushButton("⏹ 停止");
btnStop->setMinimumWidth(100);
btnStop->setMinimumHeight(35);
btnStop->setStyleSheet(
    "QPushButton { "
    "   background-color: #FFEBEE; "
    "   color: #C62828; "
    "   border: 1px solid #EF9A9A; "
    "   padding: 8px 30px; "
    "   font-size: 12px; "
    "   font-family: 'Microsoft YaHei UI'; "
    "   border-radius: 3px; "
    "} "
    "QPushButton:hover { "
    "   background-color: #FFCDD2; "
    "   border: 1px solid #E57373; "
    "} "
    "QPushButton:pressed { "
    "   background-color: #EF9A9A; "
    "}"
);

// 步进按钮（小型次要按钮）
QPushButton *btnStepForward = new QPushButton("⏭");
btnStepForward->setMinimumWidth(60);
btnStepForward->setMinimumHeight(28);
btnStepForward->setStyleSheet(
    "QPushButton { "
    "   background-color: #F5F5F5; "
    "   color: black; "
    "   border: 1px solid #CCCCCC; "
    "   padding: 5px 15px; "
    "   font-size: 11px; "
    "   font-family: 'Microsoft YaHei UI'; "
    "   border-radius: 3px; "
    "} "
    "QPushButton:hover { "
    "   background-color: #E0E0E0; "
    "} "
    "QPushButton:pressed { "
    "   background-color: #CCCCCC; "
    "}"
);
```

#### 进度条
```cpp
QProgressBar *progressBar = new QProgressBar();
progressBar->setMinimumHeight(8);
progressBar->setMaximumHeight(8);
progressBar->setTextVisible(false);
progressBar->setStyleSheet(
    "QProgressBar { "
    "   background-color: #E0E0E0; "
    "   border: none; "
    "   border-radius: 4px; "
    "} "
    "QProgressBar::chunk { "
    "   background-color: #90CAF9; "  // 使用规范中的蓝色
    "   border-radius: 4px; "
    "}"
);
```

#### 按钮栏（严格遵循规范）
```cpp
QWidget *buttonBar = new QWidget();
buttonBar->setStyleSheet("QWidget { background-color: #ECEFF1; }");
buttonBar->setMinimumHeight(60);
buttonBar->setMaximumHeight(60);

QHBoxLayout *buttonLayout = new QHBoxLayout(buttonBar);
buttonLayout->setContentsMargins(20, 10, 20, 10);
buttonLayout->addStretch();

// 全选按钮（次要按钮）
QPushButton *btnSelectAll = new QPushButton("全选");
btnSelectAll->setMinimumWidth(100);
btnSelectAll->setMinimumHeight(35);
btnSelectAll->setStyleSheet(/* 次要按钮样式 */);
buttonLayout->addWidget(btnSelectAll);

// 全不选按钮（次要按钮）
QPushButton *btnDeselectAll = new QPushButton("全不选");
btnDeselectAll->setMinimumWidth(100);
btnDeselectAll->setMinimumHeight(35);
btnDeselectAll->setStyleSheet(/* 次要按钮样式 */);
buttonLayout->addWidget(btnDeselectAll);

// 确定按钮（主要按钮）
QPushButton *btnConfirm = new QPushButton("确定");
btnConfirm->setMinimumWidth(100);
btnConfirm->setMinimumHeight(35);
btnConfirm->setStyleSheet(/* 主要按钮样式 */);
buttonLayout->addWidget(btnConfirm);
```

#### 状态栏（严格遵循规范）
```cpp
QWidget *statusBar = new QWidget();
statusBar->setMinimumHeight(30);
statusBar->setMaximumHeight(30);
statusBar->setStyleSheet("QWidget { background-color: #ECEFF1; }");

QHBoxLayout *statusLayout = new QHBoxLayout(statusBar);
statusLayout->setContentsMargins(10, 0, 10, 0);

QLabel *lblStatus = new QLabel("当前文件: test_data.blf | 就绪");
lblStatus->setStyleSheet(
    "color: black; "
    "font-size: 12px; "
    "font-family: 'Microsoft YaHei UI';"
);
statusLayout->addWidget(lblStatus);
statusLayout->addStretch();
```

---

## 3. 颜色规范总结（严格遵循UI_STANDARDS.md）

| 用途 | 颜色代码 | 说明 |
|------|----------|------|
| **标题栏背景** | #607D8B | 蓝灰色 |
| **工具栏背景** | #E8EAF6 | 浅紫蓝色（参考CANViewWidget） |
| **按钮栏背景** | #ECEFF1 | 浅灰色 |
| **状态栏背景** | #ECEFF1 | 浅灰色 |
| **内容区背景** | #FFFFFF | 白色 |
| **主要按钮** | #E3F2FD | 浅蓝色 |
| **主要按钮悬停** | #BBDEFB | 中蓝色 |
| **主要按钮按下** | #90CAF9 | 深蓝色 |
| **次要按钮** | #F5F5F5 | 浅灰色 |
| **次要按钮悬停** | #E0E0E0 | 中灰色 |
| **次要按钮按下** | #CCCCCC | 深灰色 |
| **危险按钮** | #FFEBEE | 浅红色 |
| **危险按钮悬停** | #FFCDD2 | 中红色 |
| **危险按钮按下** | #EF9A9A | 深红色 |
| **危险按钮文字** | #C62828 | 深红色 |
| **主要按钮边框** | #90CAF9 | 蓝色 |
| **次要按钮边框** | #CCCCCC | 灰色 |
| **危险按钮边框** | #EF9A9A | 红色 |
| **输入框边框** | #A4B5C8 | 蓝灰色 |
| **表格边框** | #E0E0E0 | 浅灰色 |

---

## 4. 尺寸规范总结（严格遵循UI_STANDARDS.md）

| 元素 | 尺寸 |
|------|------|
| **标题栏高度** | 40px（固定） |
| **工具栏高度** | 45px（固定） |
| **按钮栏高度** | 60px（固定） |
| **状态栏高度** | 30px（固定） |
| **对话框按钮最小宽度** | 100px |
| **对话框按钮最小高度** | 35px |
| **小型按钮最小宽度** | 60px |
| **小型按钮最小高度** | 28px |
| **按钮内边距** | 8px 30px |
| **小型按钮内边距** | 5px 15px |
| **按钮圆角** | 3px |
| **内容区边距** | 20px（上下左右） |
| **控件间距** | 15px |
| **按钮栏边距** | 上下10px，左右20px |
| **状态栏边距** | 左右10px |

---

## 5. 字体规范（严格遵循UI_STANDARDS.md）

| 用途 | 字体大小 | 字体粗细 | 字体家族 |
|------|---------|---------|---------|
| **标题栏文字** | 14px | bold | Microsoft YaHei UI |
| **正文文字** | 12px | normal | Microsoft YaHei UI |
| **小字** | 11px | normal | Microsoft YaHei UI |
| **按钮文字** | 12px | normal | Microsoft YaHei UI |

---

## 6. 检查清单

在实现UI时，请确保：

- [ ] 标题栏高度为40px，背景色#607D8B
- [ ] 工具栏高度为45px，背景色#E8EAF6
- [ ] 按钮栏高度为60px，背景色#ECEFF1
- [ ] 状态栏高度为30px，背景色#ECEFF1
- [ ] 所有按钮最小宽度100px，最小高度35px
- [ ] 按钮使用标准三态样式（正常、悬停、按下）
- [ ] 主要按钮使用#E3F2FD背景色
- [ ] 危险按钮使用#FFEBEE背景色
- [ ] 次要按钮使用#F5F5F5背景色
- [ ] 字体使用Microsoft YaHei UI
- [ ] 内容区边距20px
- [ ] 控件间距15px
- [ ] 按钮右对齐（使用addStretch()）

---

**文档版本**: v1.0  
**创建日期**: 2026-02-25  
**作者**: CANAnalyzerPro Team  
**参考**: ZCANPRO/docs/UI_STANDARDS.md
