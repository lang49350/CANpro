package org.example.utils.db;

import java.util.HashMap;
import java.util.Map;

/**
 * 表名后缀上下文（基于ThreadLocal实现线程隔离）
 */
public class TableNameContextHolder {
    // 用ThreadLocal存储当前线程的表名后缀
    private static final ThreadLocal<Map<String, String>> TABLE_SUFFIX_CONTEXT = ThreadLocal.withInitial(HashMap::new);

    /**
     * 设置表名后缀
     * @param tableName 基础表名
     * @param suffix 要添加的后缀
     */
    public static void setSuffix(String tableName, String suffix) {
        TABLE_SUFFIX_CONTEXT.get().put(tableName, suffix);
    }

    /**
     * 获取表名后缀
     * @param tableName 基础表名
     * @return 后缀字符串
     */
    public static String getSuffix(String tableName) {
        return TABLE_SUFFIX_CONTEXT.get().get(tableName);
    }

    /**
     * 清除指定表的后缀
     * @param tableName 基础表名
     */
    public static void clearSuffix(String tableName) {
        TABLE_SUFFIX_CONTEXT.get().remove(tableName);
    }

    /**
     * 清除当前线程所有表的后缀
     */
    public static void clearAll() {
        TABLE_SUFFIX_CONTEXT.remove();
    }
}
