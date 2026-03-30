# CANAnalyzerPro UI设计规范

## 📋 目录
1. [对话框规范](#对话框规范)
2. [按钮规范](#按钮规范)
3. [颜色规范](#颜色规范)
4. [字体规范](#字体规范)
5. [间距规范](#间距规范)

---

## 🪟 对话框规范

### 标准对话框结构

所有对话框应遵循以下三段式结构：

```
┌─────────────────────────────────────┐
│  1. 标题栏（蓝灰色 #607D8B）         │
├─────────────────────────────────────┤
│                                     │
│  2. 内容区域（白色背景）             │
│                                     │
│                                     │
├─────────────────────────────────────┤
│  3. 底部按钮栏（浅灰色 #ECEFF1）     │
└─────────────────────────────────────┘
```

### 1. 标题栏

```cpp
QWidget *titleBar = new QWidget();
titleBar->setMinimumHeight(40);
titleBar->setMaximumHeight(40);
titleBar->setStyleSheet("QWidget { background-color: #607D8B; }");

QHBoxLayout *titleLayout = new QHBoxLayout(titleBar);
titleLayout->setContentsMargins(15, 0, 5, 0);

QLabel *lblTitle = new QLabel("对话框标题");
lblTitle->setStyleSheet("color: white; font-size: 14px; font-weight: bold;");
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
```

**规范要点**：
- 高度：固定 40px
- 背景色：#607D8B（蓝灰色）
- 标题文字：白色、14px、加粗
- 左边距：15px
- 关闭按钮：40x40px，悬停时红色背景

### 2. 内容区域

```cpp
QWidget *contentWidget = new QWidget();
contentWidget->setStyleSheet("QWidget { background-color: white; }");

QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
contentLayout->setContentsMargins(20, 20, 20, 20);
contentLayout->setSpacing(15);
```

**规范要点**：
- 背景色：白色
- 边距：20px（上下左右）
- 控件间距：15px

### 3. 底部按钮栏

#### 标准布局（单个确定按钮）

```cpp
QWidget *buttonBar = new QWidget();
buttonBar->setStyleSheet("QWidget { background-color: #ECEFF1; }");
buttonBar->setMinimumHeight(60);
buttonBar->setMaximumHeight(60);

QHBoxLayout *buttonLayout = new QHBoxLayout(buttonBar);
buttonLayout->setContentsMargins(20, 10, 20, 10);
buttonLayout->addStretch();

QPushButton *btnConfirm = new QPushButton("确定");
btnConfirm->setMinimumWidth(100);
btnConfirm->setMinimumHeight(35);
btnConfirm->setStyleSheet(
    "QPushButton { "
    "   background-color: #E3F2FD; "
    "   color: black; "
    "   border: 1px solid #90CAF9; "
    "   padding: 8px 30px; "
    "   font-size: 12px; "
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
```

#### 双按钮布局（确定+取消）

```cpp
// ... buttonBar 和 buttonLayout 同上 ...

QPushButton *btnOk = new QPushButton("确定");
btnOk->setMinimumWidth(100);
btnOk->setMinimumHeight(35);
btnOk->setStyleSheet(/* 同上确定按钮样式 */);
buttonLayout->addWidget(btnOk);

QPushButton *btnCancel = new QPushButton("取消");
btnCancel->setMinimumWidth(100);
btnCancel->setMinimumHeight(35);
btnCancel->setStyleSheet(
    "QPushButton { "
    "   background-color: #F5F5F5; "
    "   color: black; "
    "   border: 1px solid #CCCCCC; "
    "   padding: 8px 30px; "
    "   font-size: 12px; "
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
buttonLayout->addWidget(btnCancel);
```

**规范要点**：
- 高度：固定 60px
- 背景色：#ECEFF1（浅灰色）
- 边距：上下 10px，左右 20px
- 按钮最小宽度：100px
- 按钮最小高度：35px
- 按钮间距：自动（通过addWidget）
- 按钮对齐：右对齐（使用addStretch()）

---

## 🔘 按钮规范

### 主要按钮（确定/保存）

```cpp
QPushButton *btn = new QPushButton("确定");
btn->setMinimumWidth(100);
btn->setMinimumHeight(35);
btn->setStyleSheet(
    "QPushButton { "
    "   background-color: #E3F2FD; "      // 浅蓝色
    "   color: black; "
    "   border: 1px solid #90CAF9; "
    "   padding: 8px 30px; "
    "   font-size: 12px; "
    "   border-radius: 3px; "
    "} "
    "QPushButton:hover { "
    "   background-color: #BBDEFB; "      // 中蓝色
    "   border: 1px solid #64B5F6; "
    "} "
    "QPushButton:pressed { "
    "   background-color: #90CAF9; "      // 深蓝色
    "}"
);
```

### 次要按钮（取消/关闭）

```cpp
QPushButton *btn = new QPushButton("取消");
btn->setMinimumWidth(100);
btn->setMinimumHeight(35);
btn->setStyleSheet(
    "QPushButton { "
    "   background-color: #F5F5F5; "      // 浅灰色
    "   color: black; "
    "   border: 1px solid #CCCCCC; "
    "   padding: 8px 30px; "
    "   font-size: 12px; "
    "   border-radius: 3px; "
    "} "
    "QPushButton:hover { "
    "   background-color: #E0E0E0; "      // 中灰色
    "   border: 1px solid #999999; "
    "} "
    "QPushButton:pressed { "
    "   background-color: #CCCCCC; "      // 深灰色
    "}"
);
```

### 危险按钮（删除/断开）

```cpp
QPushButton *btn = new QPushButton("删除");
btn->setMinimumWidth(100);
btn->setMinimumHeight(35);
btn->setStyleSheet(
    "QPushButton { "
    "   background-color: #FFEBEE; "      // 浅红色
    "   color: #C62828; "                 // 深红色文字
    "   border: 1px solid #EF9A9A; "
    "   padding: 8px 30px; "
    "   font-size: 12px; "
    "   border-radius: 3px; "
    "} "
    "QPushButton:hover { "
    "   background-color: #FFCDD2; "      // 中红色
    "   border: 1px solid #E57373; "
    "} "
    "QPushButton:pressed { "
    "   background-color: #EF9A9A; "      // 深红色
    "}"
);
```

### 按钮尺寸规范

| 类型 | 最小宽度 | 最小高度 | 内边距 | 字体大小 |
|------|---------|---------|--------|---------|
| 对话框按钮 | 100px | 35px | 8px 30px | 12px |
| 工具栏按钮 | 75px | - | 12px | 13px |
| 小型按钮 | 60px | 28px | 5px 15px | 11px |

---

## 🎨 颜色规范

### 主题色

| 用途 | 颜色代码 | 说明 |
|------|----------|------|
| 品牌红色 | #C8102E | Logo、强调色 |
| 蓝灰色 | #607D8B | 标题栏、通道栏 |
| 浅灰色 | #ECEFF1 | 工具栏、按钮栏背景 |
| 白色 | #FFFFFF | 内容区域背景 |

### 按钮色

| 类型 | 正常 | 悬停 | 按下 |
|------|------|------|------|
| 主要按钮 | #E3F2FD | #BBDEFB | #90CAF9 |
| 次要按钮 | #F5F5F5 | #E0E0E0 | #CCCCCC |
| 危险按钮 | #FFEBEE | #FFCDD2 | #EF9A9A |

### 边框色

| 类型 | 颜色代码 |
|------|----------|
| 主要按钮边框 | #90CAF9 |
| 次要按钮边框 | #CCCCCC |
| 危险按钮边框 | #EF9A9A |
| 输入框边框 | #A4B5C8 |

### 文字色

| 用途 | 颜色代码 |
|------|----------|
| 标题文字 | #FFFFFF（白色） |
| 正文文字 | #000000（黑色） |
| 危险文字 | #C62828（深红色） |
| 禁用文字 | rgba(43, 47, 58, 76) |

---

## 🔤 字体规范

### 字体家族

```cpp
font-family: "Microsoft YaHei UI";  // 首选
font-family: "Microsoft YaHei";     // 备选
```

### 字体大小

| 用途 | 大小 | 粗细 |
|------|------|------|
| 对话框标题 | 14px | bold |
| 正文 | 12px | normal |
| 小字 | 11px | normal |
| 工具栏按钮 | 13px | normal |

---

## 📏 间距规范

### 对话框间距

| 位置 | 间距 |
|------|------|
| 标题栏高度 | 40px |
| 按钮栏高度 | 60px |
| 状态栏高度 | 30px |
| 内容区边距 | 20px（上下左右） |
| 控件间距 | 15px |
| 按钮栏边距 | 上下 10px，左右 20px |

### 状态栏规范

```cpp
QWidget *statusBar = new QWidget();
statusBar->setMinimumHeight(30);
statusBar->setMaximumHeight(30);
statusBar->setStyleSheet("QWidget { background-color: #ECEFF1; }");

QHBoxLayout *statusLayout = new QHBoxLayout(statusBar);
statusLayout->setContentsMargins(10, 0, 10, 0);

// 状态标签
QLabel *lblStatus = new QLabel("接收帧计数: 0");
statusLayout->addWidget(lblStatus);
```

**规范要点**：
- 高度：固定 30px
- 背景色：#ECEFF1（浅灰色）
- 边距：左右 10px
- 字体：12px，正常粗细

### 按钮间距

| 位置 | 间距 |
|------|------|
| 按钮之间 | 10px（自动） |
| 按钮内边距 | 8px 30px |
| 按钮圆角 | 3px |

---

## 📝 代码模板

### 完整对话框模板

```cpp
void MyDialog::setupUi()
{
    setWindowTitle("对话框标题");
    setMinimumSize(600, 500);
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_StyledBackground, true);
    
    setObjectName("MyDialog");
    setStyleSheet(
        "QDialog#MyDialog { "
        "   background-color: white; "
        "   border: 2px solid #607D8B; "
        "   border-radius: 5px; "
        "}"
    );
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(2, 2, 2, 2);
    
    // 1. 标题栏
    QWidget *titleBar = createTitleBar();
    mainLayout->addWidget(titleBar);
    
    // 2. 内容区域
    QWidget *contentWidget = createContent();
    mainLayout->addWidget(contentWidget);
    
    // 3. 底部按钮栏
    QWidget *buttonBar = createButtonBar();
    mainLayout->addWidget(buttonBar);
}
```

---

## ✅ 检查清单

在创建新对话框时，请确保：

- [ ] 使用三段式结构（标题栏、内容区、按钮栏）
- [ ] 标题栏高度为 40px，背景色 #607D8B
- [ ] 按钮栏高度为 60px，背景色 #ECEFF1
- [ ] 状态栏高度为 30px，背景色 #ECEFF1（如需要）
- [ ] 按钮最小宽度 100px，最小高度 35px
- [ ] 按钮右对齐（使用 addStretch()）
- [ ] 使用标准按钮样式（主要/次要/危险）
- [ ] 字体使用 Microsoft YaHei UI
- [ ] 内容区边距 20px
- [ ] 控件间距 15px

---

## 📝 编码规范要点

### 文件大小限制
- ❌ 禁止：单个文件超过 **500行**
- ⚠️ 警告：单个文件超过 **300行**
- ✅ 推荐：单个文件保持在 **200-300行**

### 命名规范
- 类名：大驼峰（PascalCase）如 `CANViewWidget`
- 函数名：小驼峰（camelCase）如 `connectDevice()`
- 成员变量：`m_` 前缀如 `m_deviceName`
- 槽函数：`on` 前缀如 `onButtonClicked()`

### 注释规范
- 文件头必须包含：@file, @brief, @author, @date
- 公共类必须有注释说明职责
- 公共函数必须有 @brief 和 @param 注释
- 复杂逻辑必须有行内注释

### UI创建规范
```cpp
// ✅ 正确：分离UI创建逻辑
void MyDialog::setupUi() {
    QWidget *titleBar = createTitleBar();
    QWidget *content = createContent();
    QWidget *buttonBar = createButtonBar();
}

QWidget* MyDialog::createTitleBar() {
    // 标题栏创建逻辑
}

// ❌ 错误：所有UI代码堆在一个函数
void MyDialog::setupUi() {
    // 500行UI创建代码...
}
```

### 职责分离
- 一个类只负责一个功能领域
- UI创建、数据处理、文件操作应分离
- 超过300行立即考虑拆分

---

**最后更新**: 2026-02-11  
**版本**: v2.0 - 添加状态栏规范和编码要点
