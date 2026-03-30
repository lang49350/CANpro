package org.example.pojo.project;

import com.baomidou.mybatisplus.annotation.IdType;
import com.baomidou.mybatisplus.annotation.TableField;
import com.baomidou.mybatisplus.annotation.TableId;
import com.baomidou.mybatisplus.annotation.TableName;
import com.fasterxml.jackson.annotation.JsonFormat;
import lombok.AllArgsConstructor;
import lombok.Getter;
import lombok.NoArgsConstructor;
import lombok.Setter;
import org.example.annotation.AsColumn;
import org.example.annotation.DbName;

import java.util.Date;


@Getter
@Setter
@AllArgsConstructor
@NoArgsConstructor
@TableName("c_project")
@DbName("message")
public class CProject {
    @TableId(value = "id", type = IdType.AUTO)
    private Long id;

    @TableField(value = "name")
    @AsColumn(value = "1")
    private String name;

    @TableField(value = "create_time")
    @AsColumn(value = "2")
    private String createTime;

    @TableField(value = "create_by")
    @AsColumn(value = "3")
    private String  createBy;

    /**
     * 使用的哪个DBC,需要选择文件，可以后续在补充
     */
    @TableField(value = "dbc_metadata_id")
    private Long dbcMetadataId;

    @TableField(value = "description")
    @AsColumn(value = "4")
    private String description;

    @Override
    public String toString() {
        return "CProject{" +
                "id=" + id +
                ", name='" + name + '\'' +
                ", createTime='" + createTime + '\'' +
                ", createBy='" + createBy + '\'' +
                ", dbc_metadata_id=" + dbcMetadataId +
                ", description='" + description + '\'' +
                '}';
    }
}
