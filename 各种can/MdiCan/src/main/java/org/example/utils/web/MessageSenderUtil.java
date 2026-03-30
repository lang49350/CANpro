package org.example.utils.web;

import lombok.Data;
import org.example.pojo.data.SendMessage;
import org.example.ui.project.send.DoEventAction;
import org.example.utils.event.EventListener;
import org.example.utils.event.EventManager;
import org.example.utils.event.EventString;

import javax.swing.*;
import java.io.*;
import java.net.Socket;
import java.nio.charset.StandardCharsets;
import java.util.List;
import java.util.zip.CRC32;

@Data
public class MessageSenderUtil implements EventListener {
    private static final int BLOCK_SIZE = 64 * 1024; // 64KB

    private String boardAddressString;

    private String boardPort;

    private DoEventAction doEventAction;

    public MessageSenderUtil() {
        EventManager.getInstance().addListener(this);
    }

    /**
     * 发送消息列表到指定的开发板
     * @param ip 开发板的IP地址
     * @param port 开发板的端口号
     * @param messages 要发送的消息列表
     * @throws IOException 如果发送过程中出现网络错误
     */
    public  void sendMessages(String ip, int port, List<SendMessage> messages) throws IOException {
        try (Socket socket = new Socket(ip, port);
             OutputStream outputStream = socket.getOutputStream();
             InputStream inputStream = socket.getInputStream()) {
             System.out.println(ip + " : " + String.valueOf(port));
            // 序列化消息列表为二进制数据
            ByteArrayOutputStream byteArrayOutputStream = new ByteArrayOutputStream();
            try (ObjectOutputStream objectOutputStream = new ObjectOutputStream(byteArrayOutputStream)) {
                for (SendMessage message : messages) {
                    objectOutputStream.writeObject(message);
                }
            }
            byte[] data = byteArrayOutputStream.toByteArray();

            // 发送文件头信息
            String fileHeader = "ACK#SEND#START#" + data.length + "\n";
            outputStream.write(fileHeader.getBytes(StandardCharsets.UTF_8));
            outputStream.flush();
            System.out.println("发送文件头信息结束");

            // 添加10毫秒延迟
            try {
                Thread.sleep(10);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
                // 可以选择记录日志或执行其他操作
            }

            // 分块发送数据
            int blockCount = (int) Math.ceil((double) data.length / BLOCK_SIZE);
            EventManager.getInstance().notifyListeners(EventString.TCP_START_SEND.name() + "TOTAL:" + blockCount);
            for (int blockIndex = 0; blockIndex < blockCount; blockIndex++) {
                int start = blockIndex * BLOCK_SIZE;
                int end = Math.min(start + BLOCK_SIZE, data.length);
                byte[] block = new byte[end - start];
                System.arraycopy(data, start, block, 0, block.length);
                System.out.println("开始发送文件块 " + blockIndex);
                boolean sentSuccessfully = false;
                while (!sentSuccessfully) {
                    // 发送数据块
                    outputStream.write(block);
                    outputStream.flush();

                    System.out.println("发送第" + blockIndex + "结束");
                    // 计算并发送 CRC 校验信息
                    CRC32 crc32 = new CRC32();
                    crc32.update(block);
                    long crcValue = crc32.getValue();
                    String crcMessage = "ACK#CRC#" + blockIndex + "#" + crcValue + "\n";
                    outputStream.write(crcMessage.getBytes(StandardCharsets.UTF_8));
                    outputStream.flush();
                    System.out.println("发送第" + blockIndex + "个CRC校验值" + crcValue);
                    EventManager.getInstance().notifyListeners(EventString.TCP_START_SEND.name() + "CURRENT:" + blockIndex);

                    // 等待开发板响应
                    byte[] buffer = new byte[1024];
                    int bytesRead = inputStream.read(buffer);
                    String response = new String(buffer, 0, bytesRead);

                    System.out.println("接到开发板对数据块" + blockIndex + "的回应，" + response);

                    if (response.startsWith("ACK#CRC#" + blockIndex + "#SUCCESS#")) {
                        sentSuccessfully = true;
                        System.out.println("板子回应 CRC校验值 SUCCESS"  );
                    }else{
                        System.out.println("板子回应 CRC校验值 FAIL"  );
                    }
                    sentSuccessfully = true;
                }
                // 添加10毫秒延迟
                try {
                    Thread.sleep(10);
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                    // 可以选择记录日志或执行其他操作
                }
            }

            // 发送结束信息
            String endMessage = "ACK#SEND#END" + "\n";
            outputStream.write(endMessage.getBytes(StandardCharsets.UTF_8));
            outputStream.flush();
            JOptionPane.showMessageDialog(null, "消息发送成功！");
        }
    }

    @Override
    public void onEvent(String event) {
        System.out.println("MessageSenderUtil: " + event);
        if(event.startsWith(EventString.BOARD_CONNECT.name())){
            System.out.println(event);
            String[] split = event.split("#");
            boardAddressString = split[1];
            boardPort = split[2];
        }
    }
}