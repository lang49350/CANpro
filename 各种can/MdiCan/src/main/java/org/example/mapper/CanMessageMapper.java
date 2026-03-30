package org.example.mapper;

import com.baomidou.mybatisplus.core.mapper.BaseMapper;
import org.example.annotation.MapperPath;
import org.example.pojo.data.CanMessage;
import org.example.utils.db.BatchInsertMapper;

import java.util.List;

@MapperPath("org.example.mapper")
public interface CanMessageMapper extends BaseMapper<CanMessage> , BatchInsertMapper<CanMessage> {

}
