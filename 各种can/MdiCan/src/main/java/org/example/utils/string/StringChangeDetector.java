package org.example.utils.string;

import java.util.ArrayList;
import java.util.List;

public class StringChangeDetector {

    /**
     * 检测字符串是否按照配置符指定的规则从原始状态变为目标状态
     *
     * @param originalStr 原始字符串（变化前）
     * @param targetStr 目标字符串（变化后）
     * @param originalPattern 原始配置符（指示哪些位置应为A值）
     * @param targetPattern 目标配置符（指示哪些位置应变为B值）
     * @return 如果符合配置符指定的变化规则返回true，否则返回false
     * @throws IllegalArgumentException 当输入参数不符合要求时抛出
     */
    public static boolean detectChange(String originalStr, String targetStr,
                                       String originalPattern, String targetPattern) {
        // 验证输入参数合法性并获取关键位置
        List<Integer> keyPositions = validateAndGetKeyPositions(originalPattern, targetPattern);

        // 检查字符串长度是否与配置符匹配
        if (originalStr.length() != originalPattern.length() ||
                targetStr.length() != originalPattern.length()) {
            return false;
        }

        // 只检查关键位置
        for (int position : keyPositions) {
            char originalChar = originalPattern.charAt(position);
            char targetChar = targetPattern.charAt(position);

            // 检查原始字符串是否符合原始模式
            if (originalChar != 'X' && originalStr.charAt(position) != originalChar) {
                return false;
            }

            // 检查目标字符串是否符合目标模式
            if (targetChar != 'X' && targetStr.charAt(position) != targetChar) {
                return false;
            }
        }

        return true;
    }


    /**
     * 验证模式参数的合法性并提取关键位置
     *
     * @param originalPattern 原始配置符
     * @param targetPattern 目标配置符
     * @return 关键位置的列表
     * @throws IllegalArgumentException 当模式不符合要求时抛出
     */
    private static List<Integer> validateAndGetKeyPositions(String originalPattern, String targetPattern) {
        // 检查配置符是否为null或空
        if (originalPattern == null || originalPattern.isEmpty() ||
                targetPattern == null || targetPattern.isEmpty()) {
            throw new IllegalArgumentException("配置符不能为null或空字符串");
        }

        // 检查两个配置符长度是否相同
        if (originalPattern.length() != targetPattern.length()) {
            throw new IllegalArgumentException("两个配置符的长度必须相同");
        }

        List<Integer> keyPositions = new ArrayList<>();

        // 提取关键位置并验证
        for (int i = 0; i < originalPattern.length(); i++) {
            char originalChar = originalPattern.charAt(i);
            char targetChar = targetPattern.charAt(i);

            // 找到所有需要检查的位置（非X的位置）
            if (originalChar != 'X' || targetChar != 'X') {
                // 验证关键位置的对应关系（符合潜在条件）
                if ((originalChar != 'X' && targetChar == 'X') ||
                        (originalChar == 'X' && targetChar != 'X')) {
                    throw new IllegalArgumentException("原始配置符和目标配置符的关键位置不匹配，位置: " + i);
                }

                keyPositions.add(i);
            }
        }

        return keyPositions;
    }

    // 示例用法
    public static void main(String[] args) {
        // 配置符示例：检查第3位（索引3）是否从'1'变为'2'
        String originalPattern = "XXX1DXXXX";
        String targetPattern = "XXX2EXXXX";

        // 测试符合条件的情况
        String originalStr1 = "ABC1DEFGH";
        String targetStr1 = "ABC2EEFGH";
        System.out.println("测试1是否符合变化规则: " +
                detectChange(originalStr1, targetStr1, originalPattern, targetPattern));

        // 测试不符合条件的情况
        String originalStr2 = "ABC3DEFGH"; // 原始值不是1
        String targetStr2 = "ABC2DEFGH";
        System.out.println("测试2是否符合变化规则: " +
                detectChange(originalStr2, targetStr2, originalPattern, targetPattern));

    }
}
