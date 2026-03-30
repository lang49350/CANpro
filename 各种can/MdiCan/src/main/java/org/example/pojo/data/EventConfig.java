package org.example.pojo.data;


import com.baomidou.mybatisplus.annotation.IdType;
import com.baomidou.mybatisplus.annotation.TableField;
import com.baomidou.mybatisplus.annotation.TableId;
import com.baomidou.mybatisplus.annotation.TableName;
import lombok.AllArgsConstructor;
import lombok.Getter;
import lombok.NoArgsConstructor;
import lombok.Setter;
import org.example.annotation.DbName;

/**
 * 事件管理配置
 */

@Getter
@Setter
@AllArgsConstructor
@NoArgsConstructor
@TableName("event_config")
@DbName("message")
public class EventConfig {
    @TableId(value = "id", type = IdType.AUTO)
    private Long id;

    @TableField(value = "name")
    private String name;
    // 事件发送后的等待时间1
    @TableField(value = "wait_time")
    private String waitTime;
    // 事件发生后的等待时间2
    @TableField(value = "wait_time2")
    private String waitTime2;
    // 事件发生后的等待时间3
    @TableField(value = "wait_time3")
    private String waitTime3;
    // 事件发生后的等待时间3 delta time
    @TableField(value = "delta_time")
    private String deltaTime;

    // 是个字符串，可以包含多个消息，使用逗号分隔
    @TableField(value = "message_send_on_wait1")
    private String messageSendOnWait1;
    // 某个消息发送后的等待时间2
    @TableField(value = "message_send_on_wait2")
    private String messageSendOnWait2;

    @TableField(value = "message_send_on_wait3")
    private String messageSendOnWait3;

    @TableField(value = "event_condition")
    private String eventCondition;

    @TableField(value = "is_enable")
    private Integer isEnable;

    @TableField(value = "limit_times")
    private Integer limitTimes;

    @TableField(value = "trigger_msg_id")
    private String triggerMsgId;

    @TableField(value = "trigger_msg_from_value")
    private String triggerMsgFromValue;

    @TableField(value = "trigger_msg_to_value")
    private String triggerMsgToValue;

}
