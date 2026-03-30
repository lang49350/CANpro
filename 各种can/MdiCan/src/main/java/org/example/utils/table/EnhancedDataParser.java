package org.example.utils.table;

import lombok.extern.slf4j.Slf4j;
import org.example.pojo.data.CanMessage;
import org.example.utils.web.TextFixDefine;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
@Slf4j

public class EnhancedDataParser {
    private static final String START_MARKER = TextFixDefine.PREFIX;
    private static final String END_MARKER = TextFixDefine.SUFFIX;

    static String lastMessageIndex;
    private StringBuilder currentFrame = new StringBuilder();

    private boolean isInFrame = false;

     public  List<String> parse(byte[] data) {
         List<String> completeFrames = new ArrayList<>();
         String newData = new String(data);
         for (int i = 0; i < newData.length(); i++) {
             char c = newData.charAt(i);
             currentFrame.append(c);
             if (!isInFrame && currentFrame.toString().endsWith(START_MARKER)) {
                 isInFrame = true;
                 currentFrame.delete(0, currentFrame.length() - START_MARKER.length());
                 continue;
             }
             if(isInFrame && currentFrame.toString().endsWith(END_MARKER)){
                 isInFrame = false;
                 String frame = currentFrame.toString();
                completeFrames .add(frame);
                currentFrame.setLength(0);
             }
         }
         return completeFrames;
    }

    private static byte[] hexStringToByteArray(String s) {
        int len = s.length();
        byte[] data = new byte[len / 2];
        for (int i = 0; i < len; i += 2) {
            data[i / 2] = (byte) ((Character.digit(s.charAt(i), 16) << 4)
                    + Character.digit(s.charAt(i+1), 16));
        }
        return data;
    }

    public  CanMessage getCanMessage(byte[] data, String content, String PREFIX, String SUFFIX, String DELIMITER) {
        // 验证前缀和后缀
        if (!content.startsWith(PREFIX) || !content.endsWith(SUFFIX)) {
            log.error("数据格式错误：缺少前缀或后缀" + Arrays.toString(data));
            throw new IllegalArgumentException("数据格式错误：缺少前缀或后缀 : " + Arrays.toString(data));
        }
        // 移除前缀和后缀
        String innerContent = content.substring(PREFIX.length(), content.length() - SUFFIX.length());
        // 使用##分割数据
        String[] parts = innerContent.split(DELIMITER);
        if (parts.length < 3) {
            log.error("数据格式错误：分隔符数量不正确" + Arrays.toString(data));
            throw new IllegalArgumentException("数据格式错误：分隔符数量不正确"+ Arrays.toString(data));
        }

        // 解析16进制ID
        String canID = parts[0];
        String canData = parts[1];
        // 解析16进制长度
        Integer len;
        try {
            len = Integer.parseInt(parts[2], 16); // 以16进制解析
        } catch (NumberFormatException e) {
            log.error("数据格式错误：长度字段不是有效的16进制整数" + Arrays.toString(data));
            throw new IllegalArgumentException("长度字段不是有效的16进制整数", e);
        }

        // 解析时间搓
        String time = parts[3];

        // 解析计数器
        String counter = parts[4];

        if (lastMessageIndex == null){
            lastMessageIndex = counter;
        }
        // 验证计数器
        if(Long.parseLong(counter) - Long.parseLong(lastMessageIndex)>1){
//            log.error(lastMessageIndex + "与" + counter +"之间的数据缺失");
        }else{
            lastMessageIndex = counter;
        }
        CanMessage message = new CanMessage();
        message.setCanId(canID);
        message.setData(canData);
        message.setTime(time);
        message.setIndexCounter(Long.parseLong(counter));
        message.setDirection("接收");
        return message;
    }
}
