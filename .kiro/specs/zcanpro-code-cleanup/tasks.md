# ZCANPRO 代码清理任务清单

## 阶段 1: 准备和分析工具开发

- [ ] 1. 创建清理工具目录结构
  - [ ] 1.1 创建 ZCANPRO/tools/cleanup/ 目录
  - [ ] 1.2 创建 Python 虚拟环境
  - [ ] 1.3 安装依赖 (pathlib, re)

- [ ] 2. 开发调试代码分析工具
  - [ ] 2.1 实现 DebugCodeAnalyzer 类
  - [ ] 2.2 实现 analyze() 方法扫描所有源文件
  - [ ] 2.3 实现 categorize_debug() 分类调试输出
  - [ ] 2.4 实现 generate_report() 生成分析报告
  - [ ] 2.5 测试工具并生成初始报告

- [ ] 3. 开发文件大小分析工具
  - [ ] 3.1 实现文件行数统计脚本
  - [ ] 3.2 生成超过 500 行的文件列表
  - [ ] 3.3 按行数排序输出报告

- [ ] 4. 开发未使用代码分析工具
  - [ ] 4.1 实现 UnusedCodeAnalyzer 类
  - [ ] 4.2 实现 collect_functions() 收集函数定义
  - [ ] 4.3 实现 collect_calls() 收集函数调用
  - [ ] 4.4 实现 find_unused() 找出未使用函数
  - [ ] 4.5 测试工具并生成报告

- [ ] 5. 开发代码重复检测工具
  - [ ] 5.1 实现 DuplicateCodeAnalyzer 类
  - [ ] 5.2 实现 collect_blocks() 收集代码块
  - [ ] 5.3 实现 find_duplicates() 找出重复代码
  - [ ] 5.4 测试工具并生成报告

## 阶段 2: 调试代码清理

- [ ] 6. 分析调试代码
  - [ ] 6.1 运行 analyze_debug_code.py
  - [ ] 6.2 审查生成的报告
  - [ ] 6.3 确认清理策略 (删除/保留/转换)

- [ ] 7. 清理 MainWindow.cpp 调试代码
  - [ ] 7.1 删除装饰性 qDebug() (✅, 🔧, 📡 等)
  - [ ] 7.2 转换信息性 qDebug() 为条件编译
  - [ ] 7.3 保留错误和警告日志
  - [ ] 7.4 编译测试

- [ ] 8. 清理其他 UI 组件调试代码
  - [ ] 8.1 清理 CANViewWidget.cpp
  - [ ] 8.2 清理 FrameSenderWidget.cpp
  - [ ] 8.3 清理 DataPlaybackWidget.cpp
  - [ ] 8.4 清理 DBCViewWidget.cpp
  - [ ] 8.5 清理 J1939ViewWidget.cpp
  - [ ] 8.6 清理 WaveformViewWidget.cpp
  - [ ] 8.7 编译测试

- [ ] 9. 清理核心组件调试代码
  - [ ] 9.1 清理 SerialConnection.cpp
  - [ ] 9.2 清理 ConnectionManager.cpp
  - [ ] 9.3 清理 DataRouter.cpp
  - [ ] 9.4 清理 FramePlaybackCore.cpp
  - [ ] 9.5 编译测试

- [ ] 10. 验证调试代码清理
  - [ ] 10.1 运行 verify_cleanup.py 检查无未条件编译的 qDebug
  - [ ] 10.2 编译项目确保无错误
  - [ ] 10.3 功能测试确保程序正常运行
  - [ ] 10.4 提交更改

## 阶段 3: 大文件拆分

- [ ] 11. 拆分 MainWindow.cpp (1403 行 → 3 个文件)
  - [ ] 11.1 创建 MainWindowMenus.cpp
  - [ ] 11.2 移动所有菜单创建函数到 MainWindowMenus.cpp
  - [ ] 11.3 创建 MainWindowSubWindows.cpp
  - [ ] 11.4 移动所有子窗口管理函数到 MainWindowSubWindows.cpp
  - [ ] 11.5 更新 MainWindow.h 头文件
  - [ ] 11.6 更新 CANAnalyzerPro.pro 添加新源文件
  - [ ] 11.7 编译测试
  - [ ] 11.8 功能测试所有菜单和子窗口
  - [ ] 11.9 提交更改

- [ ] 12. 拆分 CANViewWidget.cpp (1082 行 → 3 个文件)
  - [ ] 12.1 创建 CANViewFilter.h 和 CANViewFilter.cpp
  - [ ] 12.2 实现 FilterManager 类
  - [ ] 12.3 移动过滤器相关代码到 CANViewFilter.cpp
  - [ ] 12.4 创建 CANViewExport.h 和 CANViewExport.cpp
  - [ ] 12.5 实现 ExportManager 类
  - [ ] 12.6 移动导出相关代码到 CANViewExport.cpp
  - [ ] 12.7 更新 CANViewWidget 使用新的管理器类
  - [ ] 12.8 更新 .pro 文件
  - [ ] 12.9 编译测试
  - [ ] 12.10 功能测试过滤和导出功能
  - [ ] 12.11 提交更改

- [ ] 13. 拆分 FrameSenderWidget.cpp (918 行 → 2 个文件)
  - [ ] 13.1 创建 FrameSendTaskManager.h 和 FrameSendTaskManager.cpp
  - [ ] 13.2 实现 SendTaskManager 类
  - [ ] 13.3 移动任务管理和发送逻辑到 SendTaskManager
  - [ ] 13.4 更新 FrameSenderWidget 使用 SendTaskManager
  - [ ] 13.5 更新 .pro 文件
  - [ ] 13.6 编译测试
  - [ ] 13.7 功能测试周期发送功能
  - [ ] 13.8 提交更改

- [ ] 14. 拆分 SerialConnection.cpp (902 行 → 2 个文件)
  - [ ] 14.1 创建 ProtocolHandler.h 和 ProtocolHandler.cpp
  - [ ] 14.2 实现 ProtocolHandler 类
  - [ ] 14.3 移动协议解析代码到 ProtocolHandler
  - [ ] 14.4 更新 SerialConnection 使用 ProtocolHandler
  - [ ] 14.5 更新 .pro 文件
  - [ ] 14.6 编译测试
  - [ ] 14.7 功能测试串口通信
  - [ ] 14.8 提交更改

- [ ] 15. 拆分其他超过 500 行的文件
  - [ ] 15.1 拆分 FramePlaybackCore.cpp (820 行)
  - [ ] 15.2 拆分 DataPlaybackWidget.cpp (752 行)
  - [ ] 15.3 拆分 DBCViewWidget.cpp (651 行)
  - [ ] 15.4 拆分 J1939ViewWidget.cpp (632 行)
  - [ ] 15.5 拆分 WaveformViewWidget.cpp (612 行)
  - [ ] 15.6 编译测试
  - [ ] 15.7 功能测试所有拆分的模块
  - [ ] 15.8 提交更改

- [ ] 16. 验证文件大小
  - [ ] 16.1 运行文件大小分析工具
  - [ ] 16.2 确认所有文件不超过 500 行
  - [ ] 16.3 生成文件大小报告

## 阶段 4: 未使用代码清理

- [ ] 17. 分析未使用代码
  - [ ] 17.1 运行 analyze_unused_code.py
  - [ ] 17.2 审查未使用函数列表
  - [ ] 17.3 确认哪些可以删除

- [ ] 18. 移除未使用代码
  - [ ] 18.1 删除确认未使用的函数
  - [ ] 18.2 删除未使用的类
  - [ ] 18.3 删除未使用的头文件
  - [ ] 18.4 编译测试
  - [ ] 18.5 提交更改

## 阶段 5: 代码重复优化

- [ ] 19. 分析代码重复
  - [ ] 19.1 运行 analyze_duplicates.py
  - [ ] 19.2 审查重复代码报告
  - [ ] 19.3 识别可以提取的公共函数

- [ ] 20. 优化重复代码
  - [ ] 20.1 提取重复的 UI 初始化代码
  - [ ] 20.2 提取重复的数据处理逻辑
  - [ ] 20.3 创建工具类封装公共函数
  - [ ] 20.4 更新调用代码使用公共函数
  - [ ] 20.5 编译测试
  - [ ] 20.6 功能测试
  - [ ] 20.7 提交更改

## 阶段 6: 注释代码清理

- [ ] 21. 搜索注释代码
  - [ ] 21.1 搜索大块注释代码 (连续 5 行以上)
  - [ ] 21.2 列出所有注释代码位置

- [ ] 22. 清理注释代码
  - [ ] 22.1 评估每块注释代码是否需要
  - [ ] 22.2 删除不需要的注释代码
  - [ ] 22.3 将需要保留的转为文档说明
  - [ ] 22.4 编译测试
  - [ ] 22.5 提交更改

## 阶段 7: 最终验证

- [ ] 23. 运行所有验证工具
  - [ ] 23.1 验证文件大小 (所有文件 ≤ 500 行)
  - [ ] 23.2 验证无未条件编译的调试代码
  - [ ] 23.3 验证编译成功
  - [ ] 23.4 验证无编译警告

- [ ] 24. 功能测试
  - [ ] 24.1 测试所有菜单功能
  - [ ] 24.2 测试 CAN 视图功能
  - [ ] 24.3 测试周期发送功能
  - [ ] 24.4 测试数据回放功能
  - [ ] 24.5 测试 DBC 解析功能
  - [ ] 24.6 测试 J1939 解析功能
  - [ ] 24.7 测试波形显示功能
  - [ ] 24.8 测试串口通信功能

- [ ] 25. 生成清理报告
  - [ ] 25.1 统计清理前后的代码行数
  - [ ] 25.2 统计删除的调试代码数量
  - [ ] 25.3 统计拆分的文件数量
  - [ ] 25.4 统计删除的未使用代码
  - [ ] 25.5 统计优化的重复代码
  - [ ] 25.6 生成完整清理报告

- [ ] 26. 更新文档
  - [ ] 26.1 更新 Architecture.md 反映新的文件结构
  - [ ] 26.2 更新 Code_Quality_Checklist.md
  - [ ] 26.3 添加代码清理说明文档
  - [ ] 26.4 提交文档更改

## 阶段 8: 后续维护

- [ ]* 27. 建立代码质量检查
  - [ ]* 27.1 创建 pre-commit hook 检查文件大小
  - [ ]* 27.2 创建 pre-commit hook 检查调试代码
  - [ ]* 27.3 添加 CI/CD 代码质量检查

- [ ]* 28. 团队规范
  - [ ]* 28.1 编写代码规范文档
  - [ ]* 28.2 说明文件大小限制
  - [ ]* 28.3 说明调试代码使用规范

