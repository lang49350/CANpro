package org.example.pojo.filter;

import com.baomidou.mybatisplus.annotation.IdType;
import com.baomidou.mybatisplus.annotation.TableField;
import com.baomidou.mybatisplus.annotation.TableId;
import com.baomidou.mybatisplus.annotation.TableName;
import lombok.AllArgsConstructor;
import lombok.Data;
import lombok.NoArgsConstructor;
import org.example.annotation.DbName;

import java.util.List;



@Data
@AllArgsConstructor
@NoArgsConstructor
@DbName("message")
@TableName("message_filter")
public class MessageFilter {
    //"id", "creater", "createTime", "MessageIds", "enabled"
    @TableId(value = "id",type = IdType.AUTO)
    private Integer id;
    @TableField(value = "creater")
    private String creater;
    @TableField(value = "createTime")
    private String createTime;
    @TableField(value = "MessageIds")
    private String MessageIds;
    @TableField(value = "enabled")
    private String enabled;

    @TableField(value = "project_id")
    private Long project_id;

    @Override
    public String toString() {
        return "MessageFilter{" +
                "id=" + id +
                ", creater='" + creater + '\'' +
                ", createTime='" + createTime + '\'' +
                ", MessageIds=" + MessageIds +
                ", enabled='" + enabled + '\'' +
                '}';
    }
}
