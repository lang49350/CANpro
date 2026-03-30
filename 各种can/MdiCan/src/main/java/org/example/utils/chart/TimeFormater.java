package org.example.utils.chart;

import org.example.utils.web.DateItem;

import java.time.Instant;
import java.time.ZoneId;
import java.time.ZonedDateTime;
import java.time.format.DateTimeFormatter;
import java.util.Calendar;
import java.util.Date;

public class TimeFormater {
    static DateTimeFormatter formatter = DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss.SSS");

    public static String formatTimeToHMSMS(double timeValue) {
        // 先将毫秒转换为秒
        String s = String.valueOf((int)(timeValue));
        if (s.contains(".")) {
            s = s.split("\\.")[0];
        }
        double seconds = 0.0;
        long totalMilliseconds = 0;
        if(s.length() == 10){
            seconds = timeValue * 1000;
            totalMilliseconds = (long) (seconds);
        }else{
            seconds = timeValue / 1000;
            totalMilliseconds = (long) (seconds * 1000);
        }

        // 计算总毫秒

        // 计算完整日期（用于调试）
        Calendar calendar = Calendar.getInstance();
        calendar.setTimeInMillis(totalMilliseconds);

        // 计算时、分、秒、毫秒、微秒
        long hours = calendar.get(Calendar.HOUR_OF_DAY); // 使用Calendar获取正确的小时（0-23）
        long minutes = calendar.get(Calendar.MINUTE);
        long secondsPart = calendar.get(Calendar.SECOND);
        long milliseconds = calendar.get(Calendar.MILLISECOND);
        long microseconds = (long) ((seconds - (long) seconds) * 1000000) % 1000;

        // 构建完整的时间字符串（包含日期信息用于调试）
        String dateInfo = String.format(
                "%04d-%02d-%02d",
                calendar.get(Calendar.YEAR),
                calendar.get(Calendar.MONTH) + 1,
                calendar.get(Calendar.DAY_OF_MONTH)
        );
//        System.out.println("完整时间字符串（包含日期信息）：" + dateInfo);

        // 最终格式（保留原有格式，但添加日期信息）
        return String.format(
                "%02d:%02d:%02d:%03d:%03d",
                hours, minutes, secondsPart, milliseconds, microseconds
        );
    }

    public static DateItem formatTimeToDate(String timeValue) {
        // 先将毫秒转换为秒
        long millis = convertToMillis(timeValue);
        Instant instant = Instant.ofEpochMilli(millis);
        ZonedDateTime utcDateTime = instant.atZone(ZoneId.of("UTC"));
        ZonedDateTime beijingDateTime = instant.atZone(ZoneId.of("Asia/Shanghai"));
        System.out.println("北京时间: " + beijingDateTime.format(formatter));
        // 提取年月日时分秒毫秒
        int year = beijingDateTime.getYear();
        int month = beijingDateTime.getMonthValue(); // 1-12
        int day = beijingDateTime.getDayOfMonth();
        int hour = beijingDateTime.getHour(); // 24小时制
        int minute = beijingDateTime.getMinute();
        int second = beijingDateTime.getSecond();
        int millisecond = beijingDateTime.getNano() / 1_000_000; // 纳秒转毫秒
        DateItem dateItem = new DateItem();
        dateItem.setYear(year);
        dateItem.setMonth(month);
        dateItem.setDay(day);
        dateItem.setHour(hour);
        dateItem.setMinute(minute);
        dateItem.setSecond(second);
        dateItem.setMillisecond(millisecond);
        // 转化为Date
        return dateItem;

    }

    /**
     * 将各种格式的时间戳转换为毫秒级时间戳
     * 支持：10位整数（秒）、13位整数（毫秒）、带小数的秒级时间戳（如1234567890.123）
     */
    private static long convertToMillis(String timestamp) {
        if (timestamp == null || timestamp.trim().isEmpty()) {
            throw new IllegalArgumentException("时间戳不能为空");
        }

        // 处理带小数的情况（如1753340117.916）
        if (timestamp.contains(".")) {
            String[] parts = timestamp.split("\\.");
            if (parts.length != 2) {
                throw new IllegalArgumentException("无效的带小数时间戳格式");
            }

            // 整数部分是秒，小数部分取前3位作为毫秒（不足补0，超过截断）
            long seconds = Long.parseLong(parts[0]);
            String millisPart = parts[1];
            if (millisPart.length() > 3) {
                millisPart = millisPart.substring(0, 3); // 截断超过3位的部分
            } else if (millisPart.length() < 3) {
                millisPart = String.format("%-" + 3 + "s", millisPart).replace(' ', '0'); // 不足补0
            }
            int millis = Integer.parseInt(millisPart);

            return seconds * 1000 + millis;
        } else {
            // 处理纯整数情况
            try {
                long num = Long.parseLong(timestamp);
                int length = timestamp.length();

                if (length == 10) {
                    // 10位整数：秒级，转换为毫秒
                    return num * 1000;
                } else if (length == 13) {
                    // 13位整数：直接作为毫秒级
                    return num;
                } else {
                    throw new IllegalArgumentException("不支持的整数时间戳长度（必须是10位或13位）");
                }
            } catch (NumberFormatException e) {
                throw new IllegalArgumentException("无效的整数时间戳格式");
            }
        }
    }
}

