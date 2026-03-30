package org.example.utils;

import org.jfree.data.time.TimeSeries;
import org.jfree.data.time.TimeSeriesDataItem;

public class StringUtil {
    public static String getCanId(String hexString) {
        // 检查输入是否有效
        if (hexString == null || hexString.isEmpty()) {
            throw new IllegalArgumentException("无效的十六进制字符串");
        }

        // 去掉可能的"0x"前缀
        String withoutPrefix = hexString.replace("0x", "").replace("0X", "");

        // 去掉前导零
        int firstNonZeroIndex = 0;
        while (firstNonZeroIndex < withoutPrefix.length() &&
                withoutPrefix.charAt(firstNonZeroIndex) == '0') {
            firstNonZeroIndex++;
        }

        // 如果全是零，保留一个零
        if (firstNonZeroIndex == withoutPrefix.length()) {
            return "0";
        }

        // 返回从第一个非零字符开始的子字符串
        return withoutPrefix.substring(firstNonZeroIndex);
    }

    public static void printTimeSeriesValues(TimeSeries timeSeries) {
        if (timeSeries == null) {
            System.out.println("TimeSeries 为空");
            return;
        }

        // 打印序列名称
        System.out.println("TimeSeries 名称: " + timeSeries.getKey());
        System.out.println("数据点数量: " + timeSeries.getItemCount());
        System.out.println("------------------------");

        // 遍历所有数据点并打印
        for (int i = 0; i < timeSeries.getItemCount(); i++) {
            // 获取第 i 个数据点
            TimeSeriesDataItem item = timeSeries.getDataItem(i);

            // 时间点（可以根据实际类型转换，如 Day, Hour, Minute 等）
            Object timePeriod = item.getPeriod();
            // 对应的值
            double value = item.getValue().doubleValue();

            // 打印时间点和对应的值
            System.out.printf("时间: %s, 值: %.2f%n", timePeriod, value);
        }
    }

}
