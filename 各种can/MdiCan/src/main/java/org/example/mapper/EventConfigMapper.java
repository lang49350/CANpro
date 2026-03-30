package org.example.mapper;

import com.baomidou.mybatisplus.core.mapper.BaseMapper;
import org.example.annotation.MapperPath;
import org.example.pojo.data.EventConfig;

@MapperPath("org.example.mapper")
public interface EventConfigMapper extends BaseMapper<EventConfig> {
}
