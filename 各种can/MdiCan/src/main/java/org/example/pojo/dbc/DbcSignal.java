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
import org.example.annotation.VerboserName;


@Getter
@Setter
@AllArgsConstructor
@NoArgsConstructor
@TableName("dbc_signal")
@DbName("dbc")
public class DbcSignal {
    @TableId(value = "id", type = IdType.AUTO)
    private Long id;

    @TableField(value = "name")
    @AsColumn("0")
    @VerboserName("名称")
    private String name;

    @TableField(value = "start_bit")
    @AsColumn("1")
    @VerboserName("起始位")
    private String startBit;

    @TableField(value = "length")
    @AsColumn("2")
    @VerboserName("长度")
    private String length;

    @TableField(value = "byte_order")
    @AsColumn("3")
    @VerboserName("字节序")
    private String byteOrder;

    @TableField(value = "is_signed")
    @AsColumn("4")
    @VerboserName("符号")
    private String isSigned;

    @TableField(value = "scale")
    @AsColumn("5")
    @VerboserName("比例")
    private String scale;

    @TableField(value = "offset")
    @AsColumn("6")
    @VerboserName("偏移")
    private String offset;
    @TableField(value = "minimum")
    @AsColumn("7")
    @VerboserName("最小值")
    private String minimum;

    @TableField(value = "maximum")
    @AsColumn("8")
    @VerboserName("最大值")
    private String maximum;

    @TableField(value = "unit")
    private String unit;

    @TableField(value = "receiver")
    private String receiver;

    @TableField(value = "dbc_comment")
    @AsColumn("9")
    private String comment;

    @TableField(value = "user_comment")
    @AsColumn("10")
    private String userComment;

    // DB Id
    @TableField(value = "message_id")
    private String messageId;

    @TableField(value = "dbc_metadata_id")
    private String dbcMetadataId;

}
