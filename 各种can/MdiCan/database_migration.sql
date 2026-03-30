-- 数据库迁移脚本：为can_message表添加channel列
-- 执行前请备份数据库文件

-- 添加channel列
ALTER TABLE can_message ADD COLUMN channel TEXT;

-- 为现有数据设置默认值（可选）
UPDATE can_message SET channel = 'CAN1' WHERE channel IS NULL;

-- 验证列是否添加成功
PRAGMA table_info(can_message);

