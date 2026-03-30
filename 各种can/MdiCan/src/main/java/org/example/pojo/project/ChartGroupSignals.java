package org.example.pojo.project;


import com.baomidou.mybatisplus.annotation.IdType;
import com.baomidou.mybatisplus.annotation.TableField;
import com.baomidou.mybatisplus.annotation.TableId;
import com.baomidou.mybatisplus.annotation.TableName;
import lombok.Data;
import org.example.annotation.DbName;

@Data
@TableName("chart_group_signals")
@DbName("message")
public class ChartGroupSignals {
    @TableId(value = "id", type = IdType.AUTO)
    private Long id;

    @TableField(value = "signal_name")
    private String signalName;

    // 不是数据库的ID是消息的ID
    @TableField(value = "message_id")
    private String messageId;

    @TableField(value = "message_name")
    private String messageName;

    @TableField(value = "sender")
    private String sender;

    @TableField(value = "cycle")
    private String cycle;

    @TableField(value = "max_value")
    private String maxValue;

    @TableField(value = "min_value")
    private String minValue;

    @TableField(value = "unit")
    private String unit;

    @TableField(value = "length")
    private String length;

    @TableField(value = "start_bit")
    private String startBit;

    @TableField(value = "is_signed")
    private String isSigned;

    @TableField(value = "scale")
    private String scale;

    @TableField(value = "offset")
    private String offset;

    @TableField(value = "chart_group_id")
    private String chartGroupId;

    @TableField(value = "remark")
    private Long remark;
}
