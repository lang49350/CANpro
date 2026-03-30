package org.example.mapper;

import com.baomidou.mybatisplus.core.mapper.BaseMapper;
import org.example.annotation.MapperPath;
import org.example.pojo.data.EventRecord;

@MapperPath("org.example.mapper")
public interface EventRecordMapper extends BaseMapper<EventRecord> {
}
