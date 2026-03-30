package org.example.utils.db;

import com.baomidou.mybatisplus.core.metadata.TableFieldInfo;
import com.baomidou.mybatisplus.core.metadata.TableInfo;
import com.baomidou.mybatisplus.core.metadata.TableInfoHelper;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;
import java.util.stream.IntStream;

public class BatchSqlProvider {

    public String insertBatch(Map<String, Object> params) {
        @SuppressWarnings("unchecked")
        List<Object> list = (List<Object>) params.get("list");
        if (list == null || list.isEmpty()) {
            throw new IllegalArgumentException("批量插入的列表不能为空");
        }

        // 获取实体类对应的表信息（依赖 MyBatis-Plus 注解）
        Class<?> entityClass = list.get(0).getClass();
        TableInfo tableInfo = TableInfoHelper.getTableInfo(entityClass);
        if (tableInfo == null) {
            throw new RuntimeException("未找到实体类 " + entityClass.getName() + " 的表映射信息，请检查 @TableName 注解");
        }

        // 1. 构建 INSERT 语句的字段部分（列名）
        List<TableFieldInfo> fieldList = tableInfo.getFieldList(); // 排除主键的字段列表
        String columns = fieldList.stream()
                .map(TableFieldInfo::getColumn) // 获取数据库列名
                .collect(Collectors.joining(", ", "(", ")"));

        // 2. 构建 VALUES 部分的模板（单个对象的占位符）
        String singleValueTemplate = fieldList.stream()
                .map(field -> "#{" + field.getProperty() + "}") // 实体属性名（如：#{direction}）
                .collect(Collectors.joining(", ", "(", ")"));

        // 3. 生成批量 VALUES（为每个对象的占位符加上列表索引）
        String batchValues = IntStream.range(0, list.size())
                .mapToObj(index -> {
                    // 替换单个模板中的占位符为 list[index].property（如：#{list[0].direction}）
                    return singleValueTemplate.replace("#{", "#{list[" + index + "].");
                })
                .collect(Collectors.joining(", "));

        // 4. 拼接完整 SQL
        return "INSERT INTO " + tableInfo.getTableName() + " " + columns + " VALUES " + batchValues;
    }

    /**
     * 将列表按指定批次大小拆分
     */
    public static  <T> List<List<T>> splitBatch(List<T> list, int batchSize) {
        List<List<T>> result = new ArrayList<>();
        int total = list.size();
        int batches = (total + batchSize - 1) / batchSize; // 向上取整

        for (int i = 0; i < batches; i++) {
            int start = i * batchSize;
            int end = Math.min(start + batchSize, total);
            result.add(list.subList(start, end));
        }
        return result;
    }
}
