package org.example.mapper;

import com.baomidou.mybatisplus.core.mapper.BaseMapper;
import org.example.annotation.MapperPath;
import org.example.pojo.data.SendMessage;
import org.example.utils.db.BatchInsertMapper;

@MapperPath("org.example.mapper")
public interface SendMessageMapper extends BaseMapper<SendMessage>, BatchInsertMapper<SendMessage> {
}
