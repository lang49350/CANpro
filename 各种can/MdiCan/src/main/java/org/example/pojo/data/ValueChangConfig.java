package org.example.pojo.data;

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
import org.example.annotation.VerboserName;

@Getter
@Setter
@AllArgsConstructor
@NoArgsConstructor
@TableName("value_change_config")
@DbName("message")
public class ValueChangConfig {
    @TableId(value = "id", type = IdType.AUTO)
    @AsColumn(value = "0")
    private Long id;

    // 描述
    @TableField(value = "event_description")
    @AsColumn(value = "1")
    @VerboserName("事件描述")
    private String eventDescription;

    // 事件名称
    @TableField(value = "name")
    @AsColumn(value = "2")
    @VerboserName("事件名称")
    private String name;

    // 触发消息ID
    @TableField(value = "trigger_msg_id")
    @AsColumn(value = "3")
    @VerboserName("触发消息ID")
    private String triggerMsgId;

    // 触发消息ID对应的值 from
    @TableField(value = "trigger_msg_from_value")
    @AsColumn(value = "4")
    @VerboserName("value from")
    private String valueFrom;

    // 触发消息ID对应的值 to
    @TableField(value = "trigger_msg_to_value")
    @AsColumn(value = "5")
    @VerboserName("value to")
    private String valueTo;

    // 事件发生后的等待时间1
    @TableField(value = "event_wait_ms1")
    @AsColumn(value = "6")
    @VerboserName("等待1")
    private String eventWaitMs1;

    // 触发后等待时间1
    @TableField(value = "message_send_on_wait1")
    @AsColumn(value = "7")
    @VerboserName("发送消息1")
    private String messageSendOnWait1;
    // 最大发送次数
    @TableField(value = "send_max_count1")
    @AsColumn(value = "8")
    @VerboserName("循环次数")
    private String sendMaxCount1;
    // 是否循环发送
    @TableField(value = "is_circle_send1")
    @AsColumn(value = "9")
    @VerboserName("是否循环发送")
    private String isCircleSend1;
    // 循环发送时间1
    @TableField(value = "circle_time_ms1")
    @AsColumn(value = "10")
    @VerboserName("循环等待时间")
    private String circleTimeMs1;

    // 上一个发送配置完成后的等待事件
    @TableField(value = "event_wait_ms2")
    @AsColumn(value = "11")
    @VerboserName("等待2")
    private String eventWaitMs2;

    @TableField(value = "message_send_on_wait2")
    @AsColumn(value = "12")
    @VerboserName("发送消息2")
    private String messageSendOnWait2;

    @TableField(value = "max_trigger_count")
    @AsColumn(value = "12")
    @VerboserName("最大触发次数")
    private String maxTriggerCount;


    @TableField(value = "is_enable")
    @AsColumn(value = "16")
    @VerboserName("是否启用")
    private String isEnable;

    @TableField(value = "port")
    @AsColumn(value = "16")
    @VerboserName("发送端口")
    private String port;

}

