package org.example.utils.db;

import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.nio.BufferOverflowException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.List;

import com.baomidou.mybatisplus.core.conditions.query.QueryWrapper;
import org.example.mapper.CanMessageMapper;
import org.example.pojo.data.CanMessage;

public class BlfFileGenerator {

    // 生成 BLF 文件的方法
    // BLF 文件版本
    private static final int BLF_VERSION = 0x0400;
    // 文件标志
    private static final int FILE_FLAGS = 0x00000001;
    // 对象类型：CAN 消息
    private static final int OBJECT_TYPE_CAN_MESSAGE = 0x01000000;
    // 对象头部大小
    private static final int OBJECT_HEADER_SIZE = 24;

    // 生成 BLF 文件的方法
    public static void generateBlfFile(List<CanMessage> canMessages, String filePath) {
        try (OutputStream outputStream = new FileOutputStream(filePath)) {
            // 写入完整的 BLF 文件头
            writeBlfHeader(outputStream);

            // 遍历 CAN 消息列表，写入每个消息
            for (CanMessage canMessage : canMessages) {
                writeCanMessage(outputStream, canMessage);
            }

            System.out.println("BLF file generated successfully at: " + filePath);
        } catch (IOException e) {
            System.err.println("Error generating BLF file: " + e.getMessage());
        }
    }

    // 写入完整的 BLF 文件头
    private static void writeBlfHeader(OutputStream outputStream) throws IOException {
        ByteBuffer buffer = ByteBuffer.allocate(64);
        buffer.order(ByteOrder.LITTLE_ENDIAN);

        // 签名 "BLF "
        buffer.put((byte)'B');
        buffer.put((byte)'L');
        buffer.put((byte)'F');
        buffer.put((byte)' ');

        // 文件版本
        buffer.putShort((short)BLF_VERSION);

        // 保留字段
        buffer.putShort((short)0);
        buffer.putInt(0);
        buffer.putLong(0);

        // 文件大小（暂时设为0，后续可更新）
        buffer.putLong(0);

        // 对象数量
        buffer.putLong(0); // 暂时设为0，后续可更新

        // 文件标志
        buffer.putInt(FILE_FLAGS);

        // 保留字段
        buffer.putInt(0);
        buffer.putInt(0);
        buffer.putInt(0);

        // 写入文件头
        outputStream.write(buffer.array(), 0, 64);
    }

    // 写入 CAN 消息
    // 写入 CAN 消息
    private static void writeCanMessage(OutputStream outputStream, CanMessage canMessage) throws IOException {
        // 时间戳转换为长整型
        long timestamp = Long.parseLong(canMessage.getTime());

        // 提取并验证CAN ID
        String canIdStr = canMessage.getCanId();
        // 移除可能的"0x"前缀
        if (canIdStr.startsWith("0x") || canIdStr.startsWith("0X")) {
            canIdStr = canIdStr.substring(2);
        }

        // 验证CAN ID长度是否符合标准CAN或扩展CAN
        if (canIdStr.length() > 8) {
            throw new IllegalArgumentException("CAN ID too long: " + canMessage.getCanId());
        }

        // 使用Long.parseLong处理可能的最大值
        long canIdLong = Long.parseLong(canIdStr, 16);

        // 验证CAN ID是否在有效范围内
        if (canIdLong > 0xFFFFFFFFL) {
            return;
//            throw new IllegalArgumentException("CAN ID out of range: " + canMessage.getCanId());
        }

        // 转换为int
        int canId = (int) canIdLong;

        // 数据转换为字节数组
        byte[] data = hexStringToByteArray(canMessage.getData());

        // 验证数据长度（CAN FD最大支持64字节）
        int dataLength = Math.min(data.length, 64);

        // 修正：正确判断扩展帧
        boolean isExtended = canIdLong > 0x7FF; // 标准帧ID范围是0-0x7FF

        // 计算实际需要的记录长度
        // 修正：对象头部(28) + CAN特定字段(8) + 额外保留字段(4)
        int headerSize = 28 + 8 + 4; // 明确使用28字节的对象头部
        int totalLength = headerSize + dataLength;

        // 创建 ByteBuffer 并设置字节序
        ByteBuffer buffer = ByteBuffer.allocate(totalLength);
        buffer.order(ByteOrder.LITTLE_ENDIAN);

        // 调试信息：输出缓冲区分配情况
        System.out.println("Allocating buffer with capacity: " + totalLength);

        // 写入对象头部
        buffer.putInt(totalLength);                // 对象总长度
        buffer.putInt(OBJECT_TYPE_CAN_MESSAGE);    // 对象类型
        buffer.putLong(timestamp);                 // 时间戳（100纳秒为单位）
        buffer.putLong(0);                         // 对象特定数据大小
        buffer.putInt(0);                          // 对象版本

        // 调试信息：输出对象头部写入后的位置
        System.out.println("After writing object header: position=" + buffer.position());

        // 写入CAN消息特定字段
        buffer.putInt(canId);                      // CAN ID (4 bytes)
        buffer.put((byte) dataLength);             // 数据长度码 (1 byte)

        // 设置正确的标志位
        boolean isRemote = false; // 假设不是远程帧
        boolean isError = false;  // 假设不是错误帧

        byte flags = 0;
        if (isExtended) flags |= 0x08; // 扩展帧标志
        if (isRemote) flags |= 0x10;   // 远程帧标志
        if (isError) flags |= 0x20;    // 错误帧标志

        buffer.put(flags);              // 标志位 (1 byte)

        // 扩展标志
        short extendedFlags = 0;
        // 如果是CAN FD，设置相应标志
        boolean isCanFd = dataLength > 8;
        if (isCanFd) {
            extendedFlags |= 0x0001; // FD格式标志
            extendedFlags |= 0x0002; // BRS（比特率切换）标志
        }
        buffer.putShort(extendedFlags); // 扩展标志 (2 bytes)

        // 调试信息：输出CAN特定字段写入后的位置
        System.out.println("After writing CAN specific fields: position=" + buffer.position());

        // 添加额外的保留字段（确保有足够空间）
        buffer.putInt(0); // 保留字段1

        // 调试信息：输出保留字段写入后的位置
        System.out.println("After writing reserved fields: position=" + buffer.position());

        // 检查剩余容量
        if (buffer.remaining() < dataLength) {
            System.err.println("Buffer overflow detected!");
            System.err.println("Buffer capacity: " + buffer.capacity());
            System.err.println("Bytes written: " + (buffer.position()));
            System.err.println("Data length: " + dataLength);
            System.err.println("Remaining space: " + buffer.remaining());
            throw new BufferOverflowException();
        }

        // 写入数据
        buffer.put(data, 0, dataLength);

        // 验证是否写入成功
        System.out.println("Successfully wrote message with ID: " + canMessage.getCanId()
                + ", data length: " + dataLength
                + ", total record length: " + totalLength);

        // 将 ByteBuffer 中的数据写入输出流
        outputStream.write(buffer.array());
    }
    // 十六进制字符串转换为字节数组
    private static byte[] hexStringToByteArray(String s) {
        int len = s.length();
        byte[] data = new byte[len / 2];
        for (int i = 0; i < len; i += 2) {
            data[i / 2] = (byte) ((Character.digit(s.charAt(i), 16) << 4)
                    + Character.digit(s.charAt(i + 1), 16));
        }
        return data;
    }

    public static void main(String[] args) {
        // 假设这里从数据库中获取了 CAN 消息列表
        List<CanMessage> canMessages = getCanMessagesFromDatabase();
        String filePath = "output.blf";
        generateBlfFile(canMessages, filePath);
    }

    // 从数据库获取 CAN 消息的方法，需根据实际情况实现
    private static List<CanMessage> getCanMessagesFromDatabase() {
        DatabaseManager databaseManager = new DatabaseManager();
        List<CanMessage> messages = databaseManager.execute(CanMessage.class, session -> {
            CanMessageMapper mapper = session.getMapper(CanMessageMapper.class);
            QueryWrapper<CanMessage> wrapper = new QueryWrapper<>();
            wrapper.lambda().eq(CanMessage::getProjectId, 11L);
            return mapper.selectList(wrapper);
        });
        return messages;
    }
}