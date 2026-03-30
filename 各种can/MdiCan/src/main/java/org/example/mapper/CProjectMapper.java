package org.example.mapper;

import com.baomidou.mybatisplus.core.mapper.BaseMapper;
import org.example.annotation.MapperPath;
import org.example.pojo.project.CProject;

@MapperPath("org.example.mapper")
public interface CProjectMapper extends BaseMapper<CProject> {
}
