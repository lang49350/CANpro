package org.example.utils.db;

import com.baomidou.mybatisplus.extension.plugins.handler.TableNameHandler;
/**
 * 动态表名处理器
 */
public class DynamicTableNameHandler implements TableNameHandler {
    private final String baseTableName;

    public DynamicTableNameHandler(String baseTableName) {
        this.baseTableName = baseTableName;
    }

    /**
     * 处理表名
     * @param sql 原始SQL
     * @param tableName 原始表名
     * @return 处理后的表名
     */
    @Override
    public String dynamicTableName(String sql, String tableName) {
        if(!tableName.equals(baseTableName)){
            return tableName;
        }
        // 从上下文获取当前线程设置的表名后缀
        String suffix = TableNameContextHolder.getSuffix(baseTableName);
        if (suffix != null && !suffix.isEmpty()) {
            return baseTableName + suffix;
        }
        return baseTableName;
    }
}
