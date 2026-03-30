package org.example.mapper;

import com.baomidou.mybatisplus.core.mapper.BaseMapper;
import org.example.annotation.MapperPath;
import org.example.pojo.dbc.DbcMessage;

@MapperPath("org.example.mapper")
public interface DbcMessageMapper extends BaseMapper<DbcMessage> {
}
