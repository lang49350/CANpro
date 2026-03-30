package org.example.pojo.data;

import com.baomidou.mybatisplus.annotation.IdType;
import com.baomidou.mybatisplus.annotation.TableField;
import com.baomidou.mybatisplus.annotation.TableId;
import com.baomidou.mybatisplus.annotation.TableName;
import lombok.Data;
import lombok.extern.slf4j.Slf4j;
import org.example.annotation.AsColumn;
import org.example.annotation.DbName;

import java.io.Serializable;

@Data
@TableName("send_message")
@DbName("message")
@Slf4j
public class SendMessage implements Serializable {

    private static final long serialVersionUID = 1L; // 推荐添加序列化版本UID

    @TableId(value = "id", type = IdType.AUTO)
    private Long id;

    @TableField(value = "direction")
    private String direction;

    @TableField(value = "can_id")
    @AsColumn(value = "2")
    // 16进制的字符串，4字节，例如：0x12345678
    private String canId;

    @TableField(value = "data")
    @AsColumn(value = "3")
    // 16进制的字符串，每两位代表一个字节
    private String data;

    @TableField(value = "time")
    @AsColumn(value = "0")
    // 时间，使用字符串格式储存的时间戳
    private String time;


    @TableField(value = "index_counter")
    @AsColumn(value = "4")
    private Long indexCounter ;

    /**
     * 所属的项目ID
     */
    @TableField(value = "project_id")
    // 所属的分析项目ID
    private Long projectId;
}
