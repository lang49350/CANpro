package org.example.pojo.project;


import com.baomidou.mybatisplus.annotation.IdType;
import com.baomidou.mybatisplus.annotation.TableField;
import com.baomidou.mybatisplus.annotation.TableId;
import com.baomidou.mybatisplus.annotation.TableName;
import lombok.Data;
import org.example.annotation.DbName;

@Data
@TableName("chart_group_config")
@DbName("message")
public class ChartGroupConfig {
    @TableId(value = "id", type = IdType.AUTO)
    private Long id;

    @TableField(value = "project_id")
    private Long projectId;

    @TableField(value = "name")
    private String name;

    @TableField(value = "creater")
    private String creater;

    @TableField(value = "description")
    private String description;

}
