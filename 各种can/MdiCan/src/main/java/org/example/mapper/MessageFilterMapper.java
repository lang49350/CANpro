package org.example.mapper;

import com.baomidou.mybatisplus.core.mapper.BaseMapper;
import org.example.annotation.MapperPath;
import org.example.pojo.filter.MessageFilter;

@MapperPath("org.example.mapper")
public interface MessageFilterMapper extends BaseMapper<MessageFilter> {
}
