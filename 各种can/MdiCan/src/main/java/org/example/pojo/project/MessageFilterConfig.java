package org.example.pojo.project;

import com.baomidou.mybatisplus.annotation.IdType;
import com.baomidou.mybatisplus.annotation.TableField;
import com.baomidou.mybatisplus.annotation.TableId;
import com.baomidou.mybatisplus.annotation.TableName;
import lombok.Data;
import org.example.annotation.DbName;

import java.util.Date;

@Data
@TableName(value = "message_filter_config")
@DbName("message")
public class MessageFilterConfig {
    @TableId(value = "id", type = IdType.AUTO)
    private Long id;

    @TableField(value = "project_id")
    private long projectId;

    @TableField(value = "message_id")
    private long messageId;

    @TableField(value = "enabled")
    private String enabled;

    @TableField(value = "create_time")
    private Date createTime;

    @TableField(value = "creater")
    private String creater;


}
