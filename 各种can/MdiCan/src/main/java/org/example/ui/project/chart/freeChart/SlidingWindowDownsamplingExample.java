package org.example.ui.project.chart.freeChart;

import org.jfree.chart.ChartFactory;
import org.jfree.chart.ChartPanel;
import org.jfree.chart.JFreeChart;
import org.jfree.chart.plot.PlotOrientation;
import org.jfree.data.time.FixedMillisecond;
import org.jfree.data.time.TimeSeries;
import org.jfree.data.time.TimeSeriesCollection;
import org.jfree.data.xy.XYDataset;

import javax.swing.*;
import java.awt.*;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Random;

public class SlidingWindowDownsamplingExample extends JFrame {

    public SlidingWindowDownsamplingExample(String title) {
        super(title);

        // 生成模拟的大量时间序列数据
        TimeSeries originalSeries = generateLargeTimeSeries(10000); // 10000个数据点

        // 使用滑动窗口取平均值进行稀松处理（窗口大小：10秒）
        TimeSeries downsampledSeries = downsampleByTimeWindow(originalSeries, 10);

        // 创建数据集（包含原始数据和稀松后的数据用于对比）
        TimeSeriesCollection dataset = new TimeSeriesCollection();
        dataset.addSeries(originalSeries);
        dataset.addSeries(downsampledSeries);

        // 创建图表
        JFreeChart chart = createChart(dataset);

        // 创建图表面板并添加到窗口
        ChartPanel chartPanel = new ChartPanel(chart);
        chartPanel.setPreferredSize(new Dimension(1000, 600));
        setContentPane(chartPanel);
    }

    /**
     * 生成大量模拟时间序列数据
     */
    private TimeSeries generateLargeTimeSeries(int count) {
        TimeSeries series = new TimeSeries("原始数据");
        Random random = new Random();
        long currentTime = System.currentTimeMillis();
        double value = 100.0;

        for (int i = 0; i < count; i++) {
            // 每100毫秒添加一个数据点
            currentTime += 100;
            // 模拟一个有趋势的随机值
            value += (random.nextDouble() - 0.5) * 10;
            series.add(new FixedMillisecond(currentTime), value);
        }
        return series;
    }

    /**
     * 使用滑动时间窗口对TimeSeries进行稀松处理
     * @param original 原始时间序列
     * @param windowSizeSeconds 滑动窗口大小（秒）
     * @return 稀松后的时间序列
     */
    private TimeSeries downsampleByTimeWindow(TimeSeries original, int windowSizeSeconds) {
        TimeSeries result = new TimeSeries("滑动窗口平均值（" + windowSizeSeconds + "秒）");

        if (original.getItemCount() == 0) {
            return result;
        }

        // 窗口时间范围（毫秒）
        long windowSize = windowSizeSeconds * 1000L;

        // 初始化第一个窗口
        long windowStart = original.getDataItem(0).getPeriod().getFirstMillisecond();
        long windowEnd = windowStart + windowSize;
        double sum = 0.0;
        int count = 0;

        // 遍历所有数据点
        for (int i = 0; i < original.getItemCount(); i++) {
            long itemTime = original.getDataItem(i).getPeriod().getFirstMillisecond();
            double value = original.getDataItem(i).getValue().doubleValue();

            if (itemTime < windowEnd) {
                // 当前数据点在窗口内，累加值
                sum += value;
                count++;
            } else {
                // 数据点超出当前窗口，计算平均值并添加到结果
                if (count > 0) {
                    result.add(new FixedMillisecond(windowStart), sum / count);
                }

                // 移动窗口（可能需要跳过多个窗口）
                long windowsToSkip = (itemTime - windowEnd) / windowSize + 1;
                windowStart = windowEnd + windowsToSkip * windowSize;
                windowEnd = windowStart + windowSize;

                // 添加当前数据点到新窗口
                sum = value;
                count = 1;
            }
        }

        // 添加最后一个窗口的数据
        if (count > 0) {
            result.add(new FixedMillisecond(windowStart), sum / count);
        }

        return result;
    }

    /**
     * 创建图表
     */
    private JFreeChart createChart(XYDataset dataset) {
        JFreeChart chart = ChartFactory.createTimeSeriesChart(
                "时间序列数据滑动窗口稀松示例",
                "时间",
                "数值",
                dataset,
                true,
                true,
                false
        );

        // 美化图表
        chart.getXYPlot().setOrientation(PlotOrientation.VERTICAL);

        return chart;
    }

    public static void main(String[] args) {
        // 在EDT线程中运行UI
        SwingUtilities.invokeLater(() -> {
            SlidingWindowDownsamplingExample example = new SlidingWindowDownsamplingExample(
                    "JFreeChart滑动窗口平均值稀松示例");
            example.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
            example.pack();
            example.setVisible(true);
        });
    }
}
