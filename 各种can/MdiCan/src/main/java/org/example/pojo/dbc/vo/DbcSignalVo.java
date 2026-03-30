package org.example.pojo.dbc.vo;

import com.baomidou.mybatisplus.annotation.IdType;
import com.baomidou.mybatisplus.annotation.TableField;
import com.baomidou.mybatisplus.annotation.TableId;
import lombok.Data;
import lombok.EqualsAndHashCode;
import org.example.annotation.AsColumn;
import org.example.annotation.VerboserName;
import org.example.pojo.dbc.DbcSignal;

@Data
public class DbcSignalVo{

    private Long id;

    @AsColumn("0")
    @VerboserName("名称")
    private String name;

    @AsColumn("1")
    @VerboserName("起始位")
    private String startBit;

    @AsColumn("2")
    @VerboserName("长度")
    private String length;

    @AsColumn("3")
    @VerboserName("字节序")
    private String byteOrder;

    @AsColumn("4")
    @VerboserName("符号")
    private String isSigned;

    @AsColumn("5")
    @VerboserName("比例")
    private String scale;


    @AsColumn("6")
    @VerboserName("偏移")
    private String offset;


    @AsColumn("7")
    @VerboserName("最小值")
    private String minimum;

    @AsColumn("8")
    @VerboserName("最大值")
    private String maximum;

    private String unit;

    private String receiver;

    @AsColumn("9")
    private String comment;

    @AsColumn("10")
    private String userComment;

    // DB Id
    private String messageId;

    private String dbcMetadataId;

    @AsColumn("12")
    @VerboserName("当前值")
    private String currentValue;

    @AsColumn("11")
    @VerboserName("设定值")
    private String value;


}
