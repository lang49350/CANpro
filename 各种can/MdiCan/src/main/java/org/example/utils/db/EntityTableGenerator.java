package org.example.utils.db;

import com.baomidou.mybatisplus.annotation.TableField;
import com.baomidou.mybatisplus.annotation.TableId;
import com.baomidou.mybatisplus.annotation.TableName;
import org.example.annotation.DbName;

import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.List;

public class EntityTableGenerator {

    // 获取实体类对应的数据库名称
    public static String getDatabaseName(Class<?> entityClass) {
        DbName dbNameAnnotation = entityClass.getAnnotation(DbName.class);
        if (dbNameAnnotation != null) {
            return dbNameAnnotation.value();
        }
        return "message"; // 默认数据库名
    }

    // 生成创建表的SQL语句
    public static String generateCreateTableSql(Class<?> entityClass) {
        TableName tableNameAnnotation = entityClass.getAnnotation(TableName.class);
        if (tableNameAnnotation == null) {
            throw new IllegalArgumentException("Entity class must have @TableName annotation: " + entityClass.getName());
        }

        String tableName = tableNameAnnotation.value();
        StringBuilder sql = new StringBuilder("CREATE TABLE IF NOT EXISTS ").append(tableName).append(" (");

        List<String> columns = new ArrayList<>();
        String primaryKey = null;

        // 处理类的所有字段
        for (Field field : entityClass.getDeclaredFields()) {
            TableField tableField = field.getAnnotation(TableField.class);
            TableId tableId = field.getAnnotation(TableId.class);

            if (tableField != null || tableId != null) {
                String columnName = tableField != null ? tableField.value() : field.getName();
                String columnType = getSqlType(field.getType());

                StringBuilder columnDef = new StringBuilder(columnName).append(" ").append(columnType);

                if (tableId != null) {
                    columnDef.append(" PRIMARY KEY");
                    if (tableId.type().name().contains("AUTO")) {
                        columnDef.append(" AUTOINCREMENT");
                    }
                    primaryKey = columnName;
                }

                columns.add(columnDef.toString());
            }
        }

        // 添加所有列定义
        sql.append(String.join(", ", columns));

        // 如果没有主键，添加默认主键
        if (primaryKey == null) {
            sql.append(", id INTEGER PRIMARY KEY AUTOINCREMENT");
        }

        sql.append(")");
        return sql.toString();
    }

    public static String generateCreateTableSql(Class<?> entityClass,String suffix) {
        TableName tableNameAnnotation = entityClass.getAnnotation(TableName.class);
        if (tableNameAnnotation == null) {
            throw new IllegalArgumentException("Entity class must have @TableName annotation: " + entityClass.getName());
        }

        String tableName = tableNameAnnotation.value() + suffix;
        StringBuilder sql = new StringBuilder("CREATE TABLE IF NOT EXISTS ").append(tableName).append(" (");

        List<String> columns = new ArrayList<>();
        String primaryKey = null;

        // 处理类的所有字段
        for (Field field : entityClass.getDeclaredFields()) {
            TableField tableField = field.getAnnotation(TableField.class);
            TableId tableId = field.getAnnotation(TableId.class);

            if (tableField != null || tableId != null) {
                String columnName = tableField != null ? tableField.value() : field.getName();
                String columnType = getSqlType(field.getType());

                StringBuilder columnDef = new StringBuilder(columnName).append(" ").append(columnType);

                if (tableId != null) {
                    columnDef.append(" PRIMARY KEY");
                    if (tableId.type().name().contains("AUTO")) {
                        columnDef.append(" AUTOINCREMENT");
                    }
                    primaryKey = columnName;
                }

                columns.add(columnDef.toString());
            }
        }

        // 添加所有列定义
        sql.append(String.join(", ", columns));

        // 如果没有主键，添加默认主键
        if (primaryKey == null) {
            sql.append(", id INTEGER PRIMARY KEY AUTOINCREMENT");
        }

        sql.append(")");
        return sql.toString();
    }

    // 将Java类型映射到SQLite类型
    private static String getSqlType(Class<?> javaType) {
        if (javaType == String.class) {
            return "TEXT";
        } else if (javaType == int.class || javaType == Integer.class ||
                javaType == long.class || javaType == Long.class) {
            return "INTEGER";
        } else if (javaType == double.class || javaType == Double.class ||
                javaType == float.class || javaType == Float.class) {
            return "REAL";
        } else if (javaType == boolean.class || javaType == Boolean.class) {
            return "INTEGER"; // SQLite没有布尔类型，使用0/1表示
        } else {
            return "TEXT"; // 默认使用TEXT类型
        }
    }
}