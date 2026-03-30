# ZCANPRO 代码清理设计文档

## 1. 设计概述

本文档描述 ZCANPRO 源代码清理的技术实现方案,聚焦于代码质量改进、文件拆分和调试代码清理。

## 2. 清理工具架构

### 2.1 工具结构

```
ZCANPRO/tools/cleanup/
├── analyze_debug_code.py      # 分析调试代码
├── analyze_file_size.py       # 分析文件大小
├── analyze_unused_code.py     # 分析未使用代码
├── analyze_duplicates.py      # 分析重复代码
├── refactor_split_file.py     # 文件拆分工具
└── verify_cleanup.py          # 验证清理结果
```

## 3. 调试代码清理设计

### 3.1 qDebug() 分析工具

**analyze_debug_code.py:**

```python
import re
from pathlib import Path
from typing import List, Dict

class DebugCodeAnalyzer:
    """分析代码中的调试输出"""
    
    def __init__(self, src_dir: str):
        self.src_dir = Path(src_dir)
        self.results = []
    
    def analyze(self) -> List[Dict]:
        """分析所有源文件"""
        for cpp_file in self.src_dir.rglob('*.cpp'):
            self.analyze_file(cpp_file)
        return self.results
    
    def analyze_file(self, file_path: Path):
        """分析单个文件"""
        with open(file_path, 'r', encoding='utf-8') as f:
            lines = f.readlines()
        
        for i, line in enumerate(lines, 1):
            if 'qDebug()' in line:
                category = self.categorize_debug(line)
                self.results.append({
                    'file': str(file_path),
                    'line': i,
                    'content': line.strip(),
                    'category': category
                })
    
    def categorize_debug(self, line: str) -> str:
        """分类调试输出"""
        # 装饰性输出 (emoji)
        if any(emoji in line for emoji in ['✅', '🔧', '📡', '🗑', '⚠️']):
            return 'DECORATIVE'
        
        # 错误日志
        if 'error' in line.lower() or '❌' in line:
            return 'ERROR'
        
        # 警告日志
        if 'warning' in line.lower() or '⚠' in line:
            return 'WARNING'
        
        # 信息日志
        return 'INFO'
    
    def generate_report(self) -> str:
        """生成分析报告"""
        by_category = {}
        for item in self.results:
            cat = item['category']
            by_category.setdefault(cat, []).append(item)
        
        report = "调试代码分析报告\n"
        report += "=" * 50 + "\n\n"
        
        for category, items in by_category.items():
            report += f"{category}: {len(items)} 处\n"
        
        report += "\n详细列表:\n"
        for item in self.results:
            report += f"{item['file']}:{item['line']} [{item['category']}]\n"
            report += f"  {item['content']}\n"
        
        return report
```

### 3.2 清理策略

**清理规则:**

1. **DECORATIVE (装饰性)** - 直接删除
   ```cpp
   // 删除前
   qDebug() << "✅ 主窗口初始化完成";
   
   // 删除后
   // (整行删除)
   ```

2. **INFO (信息)** - 转换为条件编译
   ```cpp
   // 转换前
   qDebug() << "创建新CAN视图，编号:" << windowId;
   
   // 转换后
   #ifdef DEBUG_MODE
   qDebug() << "创建新CAN视图，编号:" << windowId;
   #endif
   ```

3. **WARNING/ERROR** - 保留或转换为日志系统
   ```cpp
   // 转换前
   qDebug() << "❌ 启动失败";
   
   // 转换后
   qWarning() << "启动失败";
   ```

## 4. 文件拆分设计

### 4.1 MainWindow.cpp 拆分方案 (1403 行 → 3 个文件)

**目标结构:**
```
MainWindow.cpp (400 行)          - 核心窗口管理
MainWindowMenus.cpp (350 行)     - 菜单创建和处理
MainWindowSubWindows.cpp (450 行) - 子窗口管理
```

**MainWindow.h 修改:**
```cpp
class MainWindow : public QMainWindow {
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    // 菜单创建 (移到 MainWindowMenus.cpp)
    void createMenus();
    void createFileMenu(QMenuBar *menuBar);
    void createViewMenu(QMenuBar *menuBar);
    void createToolsMenu(QMenuBar *menuBar);
    void createSettingsMenu(QMenuBar *menuBar);
    void createHelpMenu(QMenuBar *menuBar);
    
    // 子窗口管理 (移到 MainWindowSubWindows.cpp)
    void onNewCANView();
    void onNewDBCView();
    void onNewJ1939View();
    void onNewWaveformView();
    void onDataPlayback();
    void onFrameSender();
    void arrangeSubWindows();
    
    // 核心功能 (保留在 MainWindow.cpp)
    void setupUI();
    void setupConnections();
    void loadSettings();
    void saveSettings();
    
private:
    // ... 成员变量
};
```

**拆分步骤:**
1. 创建 MainWindowMenus.cpp,移动所有菜单相关函数
2. 创建 MainWindowSubWindows.cpp,移动所有子窗口管理函数
3. 更新 .pro 文件添加新源文件
4. 编译测试

### 4.2 CANViewWidget.cpp 拆分方案 (1082 行 → 3 个文件)

**目标结构:**
```
CANViewWidget.cpp (400 行)       - 核心视图功能
CANViewFilter.cpp (350 行)       - 过滤器管理
CANViewExport.cpp (300 行)       - 导出功能
```

**拆分策略:**
- 提取 FilterManager 辅助类
- 提取 ExportManager 辅助类
- 保持 CANViewWidget 作为主控制器

### 4.3 FrameSenderWidget.cpp 拆分方案 (918 行 → 2 个文件)

**目标结构:**
```
FrameSenderWidget.cpp (450 行)   - UI 和用户交互
FrameSendTaskManager.cpp (450 行) - 发送任务管理
```

**SendTaskManager 类设计:**
```cpp
class SendTaskManager : public QObject {
    Q_OBJECT
    
public:
    explicit SendTaskManager(QObject *parent = nullptr);
    
    // 任务管理
    void addTask(const SendTask &task);
    void removeTask(int index);
    void updateTask(int index, const SendTask &task);
    void clearTasks();
    
    // 发送控制
    void startSending();
    void stopSending();
    void pauseSending();
    
signals:
    void taskAdded(int index);
    void taskRemoved(int index);
    void taskUpdated(int index);
    void sendingStarted();
    void sendingStopped();
    
private slots:
    void onTimerTick();
    
private:
    QList<SendTask> m_tasks;
    QTimer *m_timer;
    bool m_running;
};
```

### 4.4 SerialConnection.cpp 拆分方案 (902 行 → 2 个文件)

**目标结构:**
```
SerialConnection.cpp (450 行)    - 串口通信
ProtocolHandler.cpp (450 行)     - 协议解析
```

**ProtocolHandler 类设计:**
```cpp
class ProtocolHandler : public QObject {
    Q_OBJECT
    
public:
    explicit ProtocolHandler(QObject *parent = nullptr);
    
    // 协议处理
    void processData(const QByteArray &data);
    QByteArray encodeFrame(const CANFrame &frame);
    
signals:
    void frameReceived(const CANFrame &frame);
    void errorOccurred(const QString &error);
    
private:
    void parseCANFrame(const QByteArray &data);
    void parseCANFDFrame(const QByteArray &data);
    
    QByteArray m_buffer;
};
```

## 5. 未使用代码分析

### 5.1 静态分析工具

**analyze_unused_code.py:**

```python
import re
from pathlib import Path
from typing import Set, Dict, List

class UnusedCodeAnalyzer:
    """分析未使用的代码"""
    
    def __init__(self, src_dir: str):
        self.src_dir = Path(src_dir)
        self.functions = {}  # 函数定义
        self.calls = set()   # 函数调用
    
    def analyze(self):
        """分析所有源文件"""
        # 第一遍: 收集所有函数定义
        for cpp_file in self.src_dir.rglob('*.cpp'):
            self.collect_functions(cpp_file)
        
        # 第二遍: 收集所有函数调用
        for cpp_file in self.src_dir.rglob('*.cpp'):
            self.collect_calls(cpp_file)
        
        # 找出未被调用的函数
        return self.find_unused()
    
    def collect_functions(self, file_path: Path):
        """收集函数定义"""
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # 匹配函数定义 (简化版)
        pattern = r'^\s*\w+\s+(\w+)\s*\([^)]*\)\s*{'
        for match in re.finditer(pattern, content, re.MULTILINE):
            func_name = match.group(1)
            self.functions[func_name] = str(file_path)
    
    def collect_calls(self, file_path: Path):
        """收集函数调用"""
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # 匹配函数调用 (简化版)
        pattern = r'(\w+)\s*\('
        for match in re.finditer(pattern, content):
            func_name = match.group(1)
            self.calls.add(func_name)
    
    def find_unused(self) -> List[Dict]:
        """找出未使用的函数"""
        unused = []
        for func_name, file_path in self.functions.items():
            if func_name not in self.calls:
                # 排除特殊函数
                if not self.is_special_function(func_name):
                    unused.append({
                        'function': func_name,
                        'file': file_path
                    })
        return unused
    
    def is_special_function(self, name: str) -> bool:
        """判断是否是特殊函数 (构造/析构/Qt slots等)"""
        special = ['main', 'qMain', 'constructor', 'destructor']
        return name in special or name.startswith('on')
```

## 6. 代码重复检测

### 6.1 重复代码分析

**analyze_duplicates.py:**

```python
import hashlib
from pathlib import Path
from typing import List, Dict

class DuplicateCodeAnalyzer:
    """检测重复代码"""
    
    def __init__(self, src_dir: str, min_lines: int = 5):
        self.src_dir = Path(src_dir)
        self.min_lines = min_lines
        self.code_blocks = {}
    
    def analyze(self) -> List[Dict]:
        """分析重复代码"""
        # 收集所有代码块
        for cpp_file in self.src_dir.rglob('*.cpp'):
            self.collect_blocks(cpp_file)
        
        # 找出重复
        return self.find_duplicates()
    
    def collect_blocks(self, file_path: Path):
        """收集代码块"""
        with open(file_path, 'r', encoding='utf-8') as f:
            lines = f.readlines()
        
        # 滑动窗口提取代码块
        for i in range(len(lines) - self.min_lines + 1):
            block = ''.join(lines[i:i+self.min_lines])
            block_hash = hashlib.md5(block.encode()).hexdigest()
            
            if block_hash not in self.code_blocks:
                self.code_blocks[block_hash] = []
            
            self.code_blocks[block_hash].append({
                'file': str(file_path),
                'start_line': i + 1,
                'content': block
            })
    
    def find_duplicates(self) -> List[Dict]:
        """找出重复代码块"""
        duplicates = []
        for block_hash, locations in self.code_blocks.items():
            if len(locations) > 1:
                duplicates.append({
                    'hash': block_hash,
                    'count': len(locations),
                    'locations': locations
                })
        return duplicates
```

## 7. 验证工具设计

### 7.1 清理验证

**verify_cleanup.py:**

```python
from pathlib import Path
import subprocess

class CleanupVerifier:
    """验证清理结果"""
    
    def __init__(self, src_dir: str):
        self.src_dir = Path(src_dir)
        self.errors = []
    
    def verify_all(self) -> bool:
        """执行所有验证"""
        checks = [
            self.verify_file_sizes,
            self.verify_no_debug_code,
            self.verify_compilation,
        ]
        
        for check in checks:
            if not check():
                return False
        
        return True
    
    def verify_file_sizes(self) -> bool:
        """验证文件大小"""
        for cpp_file in self.src_dir.rglob('*.cpp'):
            line_count = sum(1 for _ in open(cpp_file, 'r', encoding='utf-8'))
            if line_count > 500:
                self.errors.append(f"{cpp_file}: {line_count} 行 (超过 500 行)")
                return False
        return True
    
    def verify_no_debug_code(self) -> bool:
        """验证无调试代码"""
        for cpp_file in self.src_dir.rglob('*.cpp'):
            with open(cpp_file, 'r', encoding='utf-8') as f:
                content = f.read()
            
            # 检查未条件编译的 qDebug
            if 'qDebug()' in content:
                # 检查是否在 #ifdef DEBUG_MODE 中
                if not self.is_conditional_debug(content):
                    self.errors.append(f"{cpp_file}: 包含未条件编译的 qDebug()")
                    return False
        return True
    
    def is_conditional_debug(self, content: str) -> bool:
        """检查 qDebug 是否在条件编译中"""
        # 简化检查: 查找 #ifdef DEBUG_MODE
        return '#ifdef DEBUG_MODE' in content or '#ifndef NDEBUG' in content
    
    def verify_compilation(self) -> bool:
        """验证编译成功"""
        result = subprocess.run(
            ['qmake', '&&', 'make'],
            cwd=self.src_dir.parent,
            capture_output=True
        )
        if result.returncode != 0:
            self.errors.append("编译失败")
            return False
        return True
```

## 8. 实施流程

### 8.1 调试代码清理流程

```
1. 运行 analyze_debug_code.py 生成报告
2. 审查报告,确认清理策略
3. 批量替换/删除调试代码
4. 编译测试
5. 提交更改
```

### 8.2 文件拆分流程

```
1. 选择目标文件 (如 MainWindow.cpp)
2. 分析文件结构,确定拆分点
3. 创建新文件,移动代码
4. 更新头文件和 .pro 文件
5. 编译测试
6. 运行功能测试
7. 提交更改
```

### 8.3 代码清理流程

```
1. 运行 analyze_unused_code.py
2. 审查未使用代码列表
3. 确认可以删除后移除
4. 运行 analyze_duplicates.py
5. 提取重复代码为公共函数
6. 编译测试
7. 提交更改
```

## 9. 总结

本设计文档提供了 ZCANPRO 代码清理的完整技术方案,包括:
- 调试代码分析和清理工具
- 大文件拆分策略和实现
- 未使用代码检测
- 代码重复分析
- 清理结果验证

通过这套工具和流程,可以系统化地改进代码质量,提高可维护性。
