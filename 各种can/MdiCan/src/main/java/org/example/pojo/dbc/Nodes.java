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

@Getter
@Setter
@AllArgsConstructor
@NoArgsConstructor
@TableName("nodes")
@DbName("dbc")
public class Nodes {
    @TableId(value = "id", type = IdType.AUTO)
    private Long id;

    @TableField(value = "name")
    private String name;

    @TableField(value = "dbc_comment")
    private String comment;

    @TableField(value = "user_comment")
    private String userComment;

    @TableField(value = "dbc_metadata_id")
    private Long dbcMetadataId;
}
