package org.example.utils.db;

import org.example.pojo.dbc.DbcSignal;
import org.example.pojo.dbc.vo.DbcSignalVo;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.List;

public class ExplanCanData {
    public static double extractSignalValue(byte[] canData, DbcSignal signal) {
        // 检查输入参数
        if (canData == null || signal == null) {
            throw new IllegalArgumentException("CAN数据和信号定义不能为空");
        }

        // 检查并转换必需的信号参数
        int startBit = parseInteger(signal.getStartBit(), "起始位");
        int length = parseInteger(signal.getLength(), "长度");
        String byteOrder = signal.getByteOrder(); // "Motorola" 或 "Intel"

        // 处理isSigned为null的情况，默认为false（无符号）
        boolean isSigned = false;
        if (signal.getIsSigned() != null && !signal.getIsSigned().trim().isEmpty()) {
            isSigned = parseInteger(signal.getIsSigned(), "是否有符号") == 1;
        }

        // 处理byteOrder为null的情况，默认为"Intel"
        if (byteOrder == null) {
            byteOrder = "Intel";
        }

        // 其余代码保持不变...
        // 计算起始字节和位偏移
        int startByte = startBit / 8;
        int bitOffset = startBit % 8;

        // 验证数据长度是否足够
        if (startByte >= canData.length) {
            throw new IllegalArgumentException("CAN数据长度不足，无法提取信号");
        }

        // 提取原始位值（使用double存储）
        double rawValue = 0;

        if (byteOrder.equalsIgnoreCase("Motorola")) {
            // Motorola位序（大端，从最高有效位开始）
            int byteCount = (length + 7) / 8;

            if (startByte + byteCount > canData.length) {
                throw new IllegalArgumentException("CAN数据长度不足，无法提取Motorola格式信号");
            }

            for (int i = 0; i < length; i++) {
                int bitPos;
                if (length <= 8) {
                    bitPos = startBit + i;
                } else {
                    int byteIndex = startByte + (i / 8);
                    int bitInByte = 7 - (i % 8);
                    bitPos = byteIndex * 8 + bitInByte;
                }

                int byteIndex = bitPos / 8;
                int bitInByte = bitPos % 8;

                int bitValue = (canData[byteIndex] >> bitInByte) & 0x01;
                // 使用double计算（注意：大数值可能会有精度损失）
                rawValue = (rawValue * 2) + bitValue;
            }
        } else {
            // Intel位序（小端，从最低有效位开始）
            for (int i = 0; i < length; i++) {
                int byteIndex = startByte + (i / 8);
                int bitInByte = bitOffset + (i % 8);

                if (bitInByte >= 8) {
                    byteIndex++;
                    bitInByte -= 8;
                }

                if (byteIndex >= canData.length) {
                    break;
                }

                int bitValue = (canData[byteIndex] >> bitInByte) & 0x01;
                // 使用double计算（注意：大数值可能会有精度损失）
                rawValue += bitValue * Math.pow(2, i);
            }
        }

        // 处理有符号值（注意：double无法直接表示有符号整数）
        if (isSigned && length > 1 && (rawValue >= Math.pow(2, length - 1))) {
            rawValue = rawValue - Math.pow(2, length);
        }

        // 应用缩放和偏移
        double scale = parseDouble(signal.getScale(), "缩放因子");
        double offset = parseDouble(signal.getOffset(), "偏移量");
        double scaledValue = rawValue * scale + offset;

        // 检查最小值和最大值限制
        Object minObj = signal.getMinimum();
        Object maxObj = signal.getMaximum();

        if (minObj instanceof Number && scaledValue < ((Number) minObj).doubleValue()) {
            scaledValue = ((Number) minObj).doubleValue();
        }

        if (maxObj instanceof Number && scaledValue > ((Number) maxObj).doubleValue()) {
            scaledValue = ((Number) maxObj).doubleValue();
        }

        return scaledValue;
    }

    // 辅助方法：安全地将字符串转换为整数
    private static int parseInteger(String value, String fieldName) {
        if (value == null || value.trim().isEmpty()) {
            throw new IllegalArgumentException(fieldName + "不能为空");
        }
        try {
            return Integer.parseInt(value.trim());
        } catch (NumberFormatException e) {
            throw new IllegalArgumentException(fieldName + "格式不正确: " + value, e);
        }
    }

    // 辅助方法：安全地将字符串转换为双精度浮点数
    private static double parseDouble(String value, String fieldName) {
        if (value == null || value.trim().isEmpty()) {
            throw new IllegalArgumentException(fieldName + "不能为空");
        }
        try {
            return Double.parseDouble(value.trim());
        } catch (NumberFormatException e) {
            throw new IllegalArgumentException(fieldName + "格式不正确: " + value, e);
        }
    }

    public static byte[] hexStringToByteArray(String hex) {
        int len = hex.length();
        byte[] data = new byte[len / 2];
        for (int i = 0; i < len; i += 2) {
            data[i / 2] = (byte) ((Character.digit(hex.charAt(i), 16) << 4)
                    + Character.digit(hex.charAt(i+1), 16));
        }
        return data;
    }

    /**
     * 根据多个 DbcSignal 生成 CAN 消息的 data
     *
     * @param signals    包含多个 DbcSignal 对象的列表，每个对象包含信号的定义信息和实际值
     * @param dataLength CAN 消息数据的长度（通常为 8 字节）
     * @return 生成的 CAN 消息数据字节数组
     */
    public static byte[] generateCanMessageData(List<DbcSignalVo> signals, int dataLength) {
        byte[] data = new byte[dataLength];

        for (DbcSignalVo signal : signals) {
            insertSignalIntoData(signal, data);
        }

        return data;
    }

    /**
     * 将单个信号的值插入到 CAN 消息数据中
     *
     * @param signal DbcSignal 对象，包含信号的定义信息和实际值
     * @param data   用于存储生成的 CAN 消息数据的字节数组
     */
    private static void insertSignalIntoData(DbcSignalVo signal, byte[] data) {
        int startBit = Integer.parseInt(signal.getStartBit());
        int length = Integer.parseInt(signal.getLength());
        double scale = Double.parseDouble(signal.getScale());
        double offset = Double.parseDouble(signal.getOffset());
        ByteOrder byteOrder = signal.getByteOrder().equals("little_endian") ? ByteOrder.LITTLE_ENDIAN : ByteOrder.BIG_ENDIAN;
        boolean isSigned = signal.getIsSigned().equals("1");
        double realValue = Double.parseDouble(signal.getValue());

        // 计算原始值
        long rawValue = (long) ((realValue - offset) / scale);

        // 创建一个 ByteBuffer 用于处理字节操作
        ByteBuffer buffer = ByteBuffer.allocate(8).order(byteOrder);
        buffer.putLong(rawValue);
        byte[] rawBytes = buffer.array();

        // 将原始值写入到 data 数组的相应位置
        for (int i = 0; i < length; i++) {
            int bitIndex = startBit + i;
            int byteIndex = bitIndex / 8;
            int bitOffset = bitIndex % 8;

            boolean bitValue = ((rawBytes[(byteOrder == ByteOrder.LITTLE_ENDIAN ? i / 8 : (length - i - 1) / 8)] >> (byteOrder == ByteOrder.LITTLE_ENDIAN ? i % 8 : (length - i - 1) % 8)) & 1) == 1;

            if (bitValue) {
                data[byteIndex] |= (byte) (1 << bitOffset);
            } else {
                data[byteIndex] &= (byte) ~(1 << bitOffset);
            }
        }
    }

    public static void main(String[] args) {
        String canDataHex1 = "0010801003084040";
        String canDataHex2 = "0010800000084040";

        byte[] canData1 = hexStringToByteArray(canDataHex1);
        byte[] canData2 = hexStringToByteArray(canDataHex2);

        DbcSignal signal = new DbcSignal();
        signal.setStartBit("28");
        signal.setLength("1");
        signal.setByteOrder("little_endian");
        signal.setScale("1");
        signal.setOffset("0");

        double value1 = extractSignalValue(canData1, signal);
        double value2 = extractSignalValue(canData2, signal);

        System.out.println("CAN数据1解析值: " + value1);
        System.out.println("CAN数据2解析值: " + value2);
    }
}