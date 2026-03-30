package org.example.service.impl;

import com.baomidou.mybatisplus.core.conditions.Wrapper;
import com.baomidou.mybatisplus.extension.service.impl.ServiceImpl;
import org.example.mapper.CanMessageMapper;
import org.example.pojo.data.CanMessage;
import org.example.service.CanMessageService;
import org.example.utils.db.DynamicTableNameHandler;
import org.example.utils.db.TableNameContextHolder;
import org.springframework.stereotype.Service;

import java.util.Collection;
import java.util.List;

@Service
public class CanMessageServiceImpl extends ServiceImpl<CanMessageMapper, CanMessage>
    implements CanMessageService {
    // 基础表名（需与数据库中基础表名一致）
    private static final String BASE_TABLE_NAME = "can_message";

    @Override
    public boolean saveDynamic(CanMessage entity, String suffix) {
        try {
            // 设置表名后缀
            TableNameContextHolder.setSuffix(BASE_TABLE_NAME, suffix);
            // 调用父类save方法（MyBatis-Plus会自动通过DynamicTableNameHandler处理表名）
            return save(entity);
        } finally {
            // 清除后缀，避免线程复用导致的问题
            TableNameContextHolder.clearSuffix(BASE_TABLE_NAME);
        }
    }

    @Override
    public boolean saveBatchDynamic(Collection<CanMessage> entityList, int batchSize, String suffix) {
        try {
            TableNameContextHolder.setSuffix(BASE_TABLE_NAME, suffix);
            return saveBatch(entityList, batchSize);
        } finally {
            TableNameContextHolder.clearSuffix(BASE_TABLE_NAME);
        }
    }

    @Override
    public boolean saveBatchDynamic(Collection<CanMessage> entityList, String suffix) {
        // 调用带默认批次大小的方法（MyBatis-Plus默认批次大小为1000）
        return saveBatchDynamic(entityList, 1000, suffix);
    }

    @Override
    public List<CanMessage> listDynamic(Wrapper<CanMessage> queryWrapper, String suffix) {
        try {
            TableNameContextHolder.setSuffix(BASE_TABLE_NAME, suffix);
            return list(queryWrapper);
        } finally {
            TableNameContextHolder.clearSuffix(BASE_TABLE_NAME);
        }
    }

    @Override
    public List<CanMessage> listDynamic(String suffix) {
        try {
            TableNameContextHolder.setSuffix(BASE_TABLE_NAME, suffix);
            // 无查询条件时，调用不带wrapper的list方法
            return list();
        } finally {
            TableNameContextHolder.clearSuffix(BASE_TABLE_NAME);
        }
    }
}
