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

/*
 * @author:
 *
 *用于记录can消息中的信号,正常这个表应该用不到，因为数据其实时存在message的data中的
 */
@Getter
@Setter
@AllArgsConstructor
@NoArgsConstructor
@TableName("can_signal")
@DbName("message")
public class CanSignal {
    @TableId(value = "id", type = IdType.AUTO)
    private Long id;

    @TableField(value = "name")
    private String name;

    @TableField(value = "start_bit")
    private String startBit;

    @TableField(value = "length")
    private String length;

    @TableField(value = "byte_order")
    private String byteOrder;

    @TableField(value = "is_signed")
    private String isSigned;

    @TableField(value = "scale")
    private String scale;

    @TableField(value = "offset")
    private String offset;

    @TableField(value = "minimum")
    private String minimum;
    @TableField(value = "maximum")
    private String maximum;
    @TableField(value = "unit")
    private String unit;

    @TableField(value = "receiver")
    private String receiver;

    @TableField(value = "dbc_comment")
    private String comment;

    @TableField(value = "message_id")
    private String messageId;

    @TableField(value = "user_comment")
    private String userComment;

    @TableField(value = "value")
    private String  value;

    // 关联CanMessage的id字段
    private Long message_db_id;

}
