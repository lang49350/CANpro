package org.example.mapper;

import com.baomidou.mybatisplus.core.mapper.BaseMapper;
import org.example.annotation.MapperPath;
import org.example.pojo.project.MessageFilterConfig;

@MapperPath("org.example.mapper")
public interface MessageFilterConfigMapper extends BaseMapper<MessageFilterConfig> {
}
