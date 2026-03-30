package org.example.pojo.dbc;


import com.baomidou.mybatisplus.annotation.IdType;
import com.baomidou.mybatisplus.annotation.TableField;
import com.baomidou.mybatisplus.annotation.TableId;
import com.baomidou.mybatisplus.annotation.TableName;
import lombok.Data;
import org.example.annotation.DbName;

import java.util.Date;

@Data
@TableName("dbc_metadata")
@DbName("dbc")
public class DbcMetadata {

    @TableId(value = "id", type = IdType.AUTO)
    private Long id;

    @TableField(value = "filename")
    private String filename;

    //文件指纹
    @TableField(value = "file_fingerprint")
    private String fileFingerprint;

    @TableField(value = "version")
    private String version;

    @TableField(value = "dbc_comment")
    private String comment;

    @TableField(value = "import_time")
    private Date importTime;
}
