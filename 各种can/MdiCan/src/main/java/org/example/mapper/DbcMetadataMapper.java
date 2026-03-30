package org.example.mapper;

import com.baomidou.mybatisplus.core.mapper.BaseMapper;
import org.example.annotation.MapperPath;
import org.example.pojo.dbc.DbcMetadata;

@MapperPath("org.example.mapper")
public interface DbcMetadataMapper extends BaseMapper<DbcMetadata> {
}
