package org.example.utils.chart;

import java.awt.Color;
import java.util.Random;

public class DistinctRandomColorGenerator {
    private static final Random random = new Random();
    private Color previousColor;

    // 亮度阈值（降低至0.6以增加颜色范围，同时保证文字可见）
    private static final double MIN_BRIGHTNESS = 0.6;
    // 颜色差异阈值（确保相邻颜色明显不同）
    private static final double COLOR_DIFFERENCE_THRESHOLD = 30.0;
    // 色相差异阈值（促进色相多样性）
    private static final double HUE_DIFFERENCE_THRESHOLD = 40.0;
    // 最大尝试次数（确保在严格条件下仍能生成足够颜色）
    private static final int MAX_ATTEMPTS = 300;

    /**
     * 获取下一个随机颜色，保持差异度和可读性的同时增加多样性
     */
    public Color getNextColor() {
        Color newColor;
        int attempts = 0;

        do {
            newColor = generateDiverseColor();
            attempts++;

            // 首次生成只需满足亮度要求
            if (previousColor == null) {
                if (isBrightEnough(newColor)) {
                    break;
                }
            } else {
                // 非首次生成需要满足三重条件
                if (isBrightEnough(newColor)
                        && calculateColorDifference(previousColor, newColor) >= COLOR_DIFFERENCE_THRESHOLD
                        && calculateHueDifference(previousColor, newColor) >= HUE_DIFFERENCE_THRESHOLD) {
                    break;
                }
            }
        } while (attempts < MAX_ATTEMPTS);

        previousColor = newColor;
        return newColor;
    }

    /**
     * 生成更多样化的随机颜色，通过HSB模型控制色彩属性
     */
    private Color generateDiverseColor() {
        // 50%概率生成高饱和度颜色（鲜艳），50%生成中等饱和度（柔和）
        if (random.nextBoolean()) {
            // 高饱和度策略：饱和度70%-90%，亮度70%-95%
            float hue = random.nextFloat();
            float saturation = 0.7f + random.nextFloat() * 0.2f;
            float brightness = 0.7f + random.nextFloat() * 0.25f;
            return Color.getHSBColor(hue, saturation, brightness);
        } else {
            // 中等饱和度策略：饱和度30%-60%，亮度65%-95%
            float hue = random.nextFloat();
            float saturation = 0.3f + random.nextFloat() * 0.3f;
            float brightness = 0.65f + random.nextFloat() * 0.3f;
            return Color.getHSBColor(hue, saturation, brightness);
        }
    }

    /**
     * 检查颜色亮度是否满足最低要求（确保黑色文字可见）
     */
    private boolean isBrightEnough(Color color) {
        // 提取RGB值并归一化到0-1范围
        double r = color.getRed() / 255.0;
        double g = color.getGreen() / 255.0;
        double b = color.getBlue() / 255.0;

        // 计算相对亮度（符合WCAG 2.0标准）
        double rLinear = (r <= 0.03928) ? r / 12.92 : Math.pow((r + 0.055) / 1.055, 2.4);
        double gLinear = (g <= 0.03928) ? g / 12.92 : Math.pow((g + 0.055) / 1.055, 2.4);
        double bLinear = (b <= 0.03928) ? b / 12.92 : Math.pow((b + 0.055) / 1.055, 2.4);

        double luminance = 0.2126 * rLinear + 0.7152 * gLinear + 0.0722 * bLinear;
        return luminance >= MIN_BRIGHTNESS;
    }

    /**
     * 计算两个颜色的色相差异（0-180度）
     */
    private double calculateHueDifference(Color color1, Color color2) {
        float[] hsb1 = new float[3];
        float[] hsb2 = new float[3];

        // 转换为HSB颜色空间
        Color.RGBtoHSB(color1.getRed(), color1.getGreen(), color1.getBlue(), hsb1);
        Color.RGBtoHSB(color2.getRed(), color2.getGreen(), color2.getBlue(), hsb2);

        // 色相值范围是0-1，转换为0-360度
        double hue1 = hsb1[0] * 360;
        double hue2 = hsb2[0] * 360;

        // 计算最小色相差异（取环上最短距离）
        double diff = Math.abs(hue1 - hue2);
        return Math.min(diff, 360 - diff);
    }

    /**
     * 计算两个颜色的整体差异（CIE76公式，反映人眼感知差异）
     */
    private double calculateColorDifference(Color color1, Color color2) {
        double[] lab1 = rgbToLab(color1);
        double[] lab2 = rgbToLab(color2);

        double deltaL = lab1[0] - lab2[0];
        double deltaA = lab1[1] - lab2[1];
        double deltaB = lab1[2] - lab2[2];

        return Math.sqrt(deltaL * deltaL + deltaA * deltaA + deltaB * deltaB);
    }

    /**
     * 将RGB颜色转换为CIE Lab颜色空间（用于更准确的色差计算）
     */
    private double[] rgbToLab(Color color) {
        double r = color.getRed() / 255.0;
        double g = color.getGreen() / 255.0;
        double b = color.getBlue() / 255.0;

        // 伽马校正
        r = (r > 0.04045) ? Math.pow((r + 0.055) / 1.055, 2.4) : r / 12.92;
        g = (g > 0.04045) ? Math.pow((g + 0.055) / 1.055, 2.4) : g / 12.92;
        b = (b > 0.04045) ? Math.pow((b + 0.055) / 1.055, 2.4) : b / 12.92;

        // 转换到XYZ颜色空间
        double x = r * 0.4124 + g * 0.3576 + b * 0.1805;
        double y = r * 0.2126 + g * 0.7152 + b * 0.0722;
        double z = r * 0.0193 + g * 0.1192 + b * 0.9505;

        // 参考白点（D65标准光源）
        double xRef = 0.95047;
        double yRef = 1.0;
        double zRef = 1.08883;

        x /= xRef;
        y /= yRef;
        z /= zRef;

        // 转换到Lab颜色空间
        x = (x > 0.008856) ? Math.pow(x, 1.0/3.0) : (7.787 * x) + 16.0/116.0;
        y = (y > 0.008856) ? Math.pow(y, 1.0/3.0) : (7.787 * y) + 16.0/116.0;
        z = (z > 0.008856) ? Math.pow(z, 1.0/3.0) : (7.787 * z) + 16.0/116.0;

        double l1 = (116 * y) - 16;
        double a1 = 500 * (x - y);
        double b1 = 200 * (y - z);

        return new double[] {l1, a1, b1};
    }

    // 测试方法
    public static void main(String[] args) {
        DistinctRandomColorGenerator generator = new DistinctRandomColorGenerator();
        for (int i = 0; i < 15; i++) {
            Color color = generator.getNextColor();
            System.out.printf("颜色 %d: R=%d, G=%d, B=%d, 亮度=%.2f%n",
                    i+1,
                    color.getRed(),
                    color.getGreen(),
                    color.getBlue(),
                    calculateBrightness(color)
            );
        }
    }

    // 辅助方法：计算颜色亮度（用于测试输出）
    private static double calculateBrightness(Color color) {
        double r = color.getRed() / 255.0;
        double g = color.getGreen() / 255.0;
        double b = color.getBlue() / 255.0;

        double rLinear = (r <= 0.03928) ? r / 12.92 : Math.pow((r + 0.055) / 1.055, 2.4);
        double gLinear = (g <= 0.03928) ? g / 12.92 : Math.pow((g + 0.055) / 1.055, 2.4);
        double bLinear = (b <= 0.03928) ? b / 12.92 : Math.pow((b + 0.055) / 1.055, 2.4);

        return 0.2126 * rLinear + 0.7152 * gLinear + 0.0722 * bLinear;
    }
}