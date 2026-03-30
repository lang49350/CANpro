package org.example.pojo.dbc;

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

/**
 *  读取解读的DBC文件中的消息
 */

@Getter
@Setter
@AllArgsConstructor
@NoArgsConstructor
@TableName("dbc_message")
@DbName("dbc")
public class DbcMessage {
    @TableId(value = "id", type = IdType.AUTO)
    private Long id;

    @TableField(value = "message_id")
    @AsColumn("0")
    private String messageId;

    @TableField(value = "name")
    @AsColumn("1")
    private String name;

    @TableField(value = "length")
    @AsColumn("2")
    private String length;

    @TableField(value = "cycle_time")
    @AsColumn("3")
    private String cycleTime;

    @TableField(value = "send_type")
    @AsColumn("4")
    private String sendType;

    @TableField(value = "sender")
    @AsColumn("5")
    private String sender;

    @TableField(value = "dbc_comment")
    @AsColumn("6")
    private String comment;

    @TableField(value = "user_comment")
    @AsColumn("7")
    private String userComment;

    @TableField(value = "dbc_metadata_id")
    private Long dbcMetadataId;

}
