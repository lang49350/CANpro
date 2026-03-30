package org.example.pojo.project;

import com.baomidou.mybatisplus.annotation.IdType;
import com.baomidou.mybatisplus.annotation.TableField;
import com.baomidou.mybatisplus.annotation.TableId;
import com.baomidou.mybatisplus.annotation.TableName;
import lombok.AllArgsConstructor;
import lombok.Getter;
import lombok.NoArgsConstructor;
import lombok.Setter;
import org.example.annotation.AsColumn;
import org.example.annotation.DbName;

import java.time.LocalDateTime;

@Getter
@Setter
@AllArgsConstructor
@NoArgsConstructor
@TableName("signal_group")
@DbName("message")
public class SignalGroup {
    @TableId(value = "id", type = IdType.AUTO)
    @AsColumn(value = "1")
    private Integer id;

    @TableField(value = "name")
    @AsColumn(value = "2")
    private String name;

    @TableField(value = "creator")
    @AsColumn(value = "3")
    private String creator;

    @TableField(value = "creation_time")
    @AsColumn(value = "4")
    private String creationTime;

    @TableField(value = "remark")
    @AsColumn(value = "5")
    private String remark;

}    