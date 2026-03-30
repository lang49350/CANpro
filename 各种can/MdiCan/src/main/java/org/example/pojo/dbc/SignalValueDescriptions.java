package org.example.pojo.dbc;

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
 * 信号值描述
 */

@Getter
@Setter
@AllArgsConstructor
@NoArgsConstructor
@TableName("signal_value_descriptions")
@DbName("dbc")
public class SignalValueDescriptions {

    @TableId(value = "id", type = IdType.AUTO)
    private Long id;

    @TableField(value = "signal_id")
    private long signalId;

    @TableField(value = "value")
    private String value;

    @TableField(value = "description")
    private String description;

    @TableField(value = "user_comment")
    private String userComment;

    @TableField(value = "dbc_metadata_id")
    private Long dbcMetadataId;

}
