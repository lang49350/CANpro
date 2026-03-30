package org.example.mapper;

import com.baomidou.mybatisplus.core.mapper.BaseMapper;
import org.example.annotation.MapperPath;
import org.example.pojo.project.SignalInGroup;
import org.example.utils.db.BatchInsertMapper;

@MapperPath("org.example.mapper")
public interface SignalInGroupMapper extends BaseMapper<SignalInGroup>, BatchInsertMapper<SignalInGroup> {
}
