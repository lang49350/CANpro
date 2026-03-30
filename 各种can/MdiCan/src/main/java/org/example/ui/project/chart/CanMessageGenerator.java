package org.example.ui.project.chart;


/**
 *  生成测试数据的工具
 */

import org.example.pojo.data.CanMessage;

import java.text.SimpleDateFormat;
import java.time.Instant;
import java.time.LocalDateTime;
import java.time.ZoneId;
import java.time.format.DateTimeFormatter;
import java.util.*;

public class CanMessageGenerator {
    private final String canId;
    private final Random random;

    private long lastTimestamp = System.currentTimeMillis(); // 记录上一次生成的时间戳

    public CanMessageGenerator(String canId) {
        this.canId = canId;
        this.random = new Random();
    }

    /**
     * 生成随机的十六进制字符串数据
     * @param length 数据长度
     * @return 十六进制字符串
     */
    private String generateRandomHexData(int length) {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < length; i++) {
            int randomInt = random.nextInt(256);
            sb.append(String.format("%02X", randomInt));
        }
        return sb.toString();
    }

    /**
     * 生成单个 CanMessage 对象
     * @return CanMessage 对象
     */
    public CanMessage generateSingleMessage() {
        CanMessage message = new CanMessage();
        message.setDirection("RandomDirection"); // 随机方向
        message.setCanId(canId);
        message.setData(generateRandomHexData(8)); // 假设数据长度为 8 字节
//        message.setTime(String.valueOf(System.currentTimeMillis())); // 使用当前时间戳
        // 生成新的时间戳（确保与上一个时间戳的间隔至少为400ms，并允许未来时间）
        long minNextTime = lastTimestamp + 400;
        long currentTime = System.currentTimeMillis();
        long nextTimestamp = Math.max(minNextTime, currentTime + random.nextInt(500)); // 允许最多500ms的未来时间

        message.setTime(String.valueOf(nextTimestamp));
        lastTimestamp = nextTimestamp; // 更新上一次时间戳
        message.setIndexCounter((long) random.nextInt(1000)); // 随机索引计数器
        message.setProjectId((long) random.nextInt(100)); // 随机项目 ID
        return message;
    }

    /**
     * 持续生成 CanMessage 数据
     * @param count 生成的消息数量
     * @return CanMessage 列表
     */
    public List<CanMessage> generateMessages(int count) {
        List<CanMessage> messages = new ArrayList<>();
        for (int i = 0; i < count; i++) {
            messages.add(generateSingleMessage());
        }
        return messages;
    }

    public static void main(String[] args) {
        String canId = "2A6";
        CanMessageGenerator generator = new CanMessageGenerator(canId);
        List<CanMessage> messages = generator.generateMessages(100); // 生成 10 条消息
        DateTimeFormatter formatter = DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss:SSS");
        for (CanMessage message : messages) {
            System.out.println(message);
            Instant instant = Instant.ofEpochMilli(Long.parseLong(message.getTime()));
            LocalDateTime localDateTime = LocalDateTime.ofInstant(instant, ZoneId.systemDefault());
            System.out.println(localDateTime.format(formatter));
        }
    }
}