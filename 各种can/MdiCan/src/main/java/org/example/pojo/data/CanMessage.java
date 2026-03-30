package org.example.pojo.data;

import com.alibaba.excel.annotation.ExcelProperty;
import com.baomidou.mybatisplus.annotation.IdType;
import com.baomidou.mybatisplus.annotation.TableField;
import com.baomidou.mybatisplus.annotation.TableId;
import com.baomidou.mybatisplus.annotation.TableName;
import lombok.*;
import lombok.extern.slf4j.Slf4j;
import org.apache.commons.lang3.StringUtils;
import org.example.annotation.AsColumn;
import org.example.annotation.DbName;
import org.example.utils.StringUtil;
import org.example.utils.table.EnhancedDataParser;
import org.example.utils.web.TextFixDefine;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * @author 17694
 * @version 1.0
 * @description:  用于记录接收到的CAN消息
 * @date 2024/10/18 16:34
 */
@Data
@TableName("can_message")
@DbName("message")
@Slf4j
public class CanMessage {
    @TableId(value = "id", type = IdType.AUTO)
    private Long id;

    @TableField(value = "direction")
    @AsColumn(value = "1")
    @ExcelProperty("方向")
    private String direction;

    @TableField(value = "can_id")
    @AsColumn(value = "2")
    @ExcelProperty("CAN ID")
    // 16进制的字符串，4字节，例如：0x12345678
    private String canId;

    @TableField(value = "data")
    @ExcelProperty("数据")
    @AsColumn(value = "3")
    // 16进制的字符串，每两位代表一个字节
    private String data;

    @TableField(value = "time")
    @AsColumn(value = "0")
    @ExcelProperty("时间")
    // 时间，使用字符串格式储存的时间戳
    private String time;


    @TableField(value = "index_counter")
    @AsColumn(value = "4")
    @ExcelProperty("索引计数器")
    private Long indexCounter ;

    /**
     * 所属的项目ID
     */
    @TableField(value = "project_id")
    // 所属的分析项目ID
    private Long projectId;

    public static List<CanMessage> parse(byte[] data) {
        CanMessage message = new CanMessage();
//        String PREFIX = "*****##";
        String PREFIX = TextFixDefine.PREFIX;
//        String SUFFIX = "##*****"; // 修改后的SUFFIX
        String SUFFIX = TextFixDefine.SUFFIX; // 修改后的SUFFIX
        String DELIMITER = TextFixDefine.DELIMITER;
//        String DELIMITER = "##";
        if (data == null || data.length == 0) {
            log.error("输入数据不能为空" + Arrays.toString(data));
            throw new IllegalArgumentException("输入数据不能为空" + Arrays.toString(data));
        }
        EnhancedDataParser enhancedDataParser = new EnhancedDataParser();
        List<String> stringList = enhancedDataParser.parse(data);
        List<CanMessage> messages = new ArrayList<>();
        for (String s : stringList) {
            if(s.equals("*****#")){
                System.out.println("*****#");
            }
            CanMessage canMessage = enhancedDataParser.getCanMessage(s.getBytes(), s, PREFIX, SUFFIX, DELIMITER);
            System.out.println(canMessage);
            messages.add(canMessage);
        }
        return messages;
    }


    public static List<CanMessage> toCanMessageList(List<String> dataList) {
        List<CanMessage> canMessages = new ArrayList<>();
        for(String data:dataList){
            if(StringUtils.isBlank(data) || data.length()<3){
                continue;
            }
            CanMessage canMessage = new CanMessage();
            String[] split = data.split(",");
            String canId = split[0];
            String dataStr = split[1];
            String timeStr = split[2];
            String index = split[3];
            canMessage.setCanId(canId);
            canMessage.setData(dataStr);
            canMessage.setTime(timeStr);
            canMessage.setIndexCounter(Long.parseLong(StringUtils.defaultIfBlank(index,"0")));
            canMessages.add(canMessage);
        }
        return canMessages;
    }
}
