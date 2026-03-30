package org.example.ui.project.chart.freeChart;

import lombok.Data;
import lombok.EqualsAndHashCode;
import lombok.extern.slf4j.Slf4j;
import org.jfree.chart.ChartFactory;
import org.jfree.chart.ChartPanel;
import org.jfree.chart.JFreeChart;
import org.jfree.chart.axis.DateAxis;
import org.jfree.chart.axis.NumberAxis;
import org.jfree.chart.labels.StandardXYItemLabelGenerator;
import org.jfree.chart.plot.XYPlot;
import org.jfree.chart.renderer.xy.XYLineAndShapeRenderer;
import org.jfree.data.time.*;

import javax.swing.*;
import java.awt.*;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.awt.event.MouseMotionAdapter;
import java.awt.geom.Ellipse2D;
import java.awt.geom.Rectangle2D;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Random;
import java.util.concurrent.atomic.AtomicInteger;

@EqualsAndHashCode(callSuper = true)
@Data
@Slf4j
public class DraggableTimeChart extends JPanel {
    private TimeSeries dataSeries;
    private static final int MAX_DATA_POINTS = 500; // 最大数据点数量

    private boolean isDragging = false;
    private int dragStartX, dragStartY;
    private int panelStartX, panelStartY;

    // 调整大小相关变量
    private boolean isResizing = false;
    private int resizeStartX, resizeStartY;
    private Dimension startSize;
    private static final int RESIZE_BORDER = 8; // 边界区域大小
    ChartPanel chartPanel;
    private String title;
    private JFreeChart chart;
    private XYLineAndShapeRenderer renderer;
    private JButton closeButton; // 关闭按钮
    private Timer timer;
    private boolean isShowValue = false;
    // 是否采用稀松模式
    private boolean isDownSampling = false;


    public DraggableTimeChart(String title, Color color) {
        init(title, color);
    }

    private void init(String title, Color color) {
        // 设置布局管理器，让图表面板能自动填充空间
        setLayout(new BorderLayout());
        setBorder(BorderFactory.createTitledBorder(title));
        setBackground(color);
        setPreferredSize(new Dimension(200, 150));

        initCloseButton();

        // 添加鼠标监听器处理拖动和调整大小
        addMouseListener(new MouseAdapter() {
            @Override
            public void mousePressed(MouseEvent e) {
                // 检查是否在边界区域（调整大小）
                if (isInResizeArea(e)) {
                    isResizing = true;
                    resizeStartX = e.getXOnScreen();
                    resizeStartY = e.getYOnScreen();
                    startSize = getSize();
                    setCursor(Cursor.getPredefinedCursor(Cursor.SE_RESIZE_CURSOR));
                } else {
                    // 否则处理拖动
                    isDragging = true;
                    dragStartX = e.getXOnScreen();
                    dragStartY = e.getYOnScreen();
                    panelStartX = getX();
                    panelStartY = getY();
                    setCursor(Cursor.getPredefinedCursor(Cursor.MOVE_CURSOR));
                }
            }

            @Override
            public void mouseReleased(MouseEvent e) {
                isDragging = false;
                isResizing = false;
                setCursor(Cursor.getDefaultCursor());
            }
        });

        addMouseMotionListener(new MouseMotionAdapter() {
            @Override
            public void mouseDragged(MouseEvent e) {
                if (isResizing) {
                    // 处理调整大小
                    int dx = e.getXOnScreen() - resizeStartX;
                    int dy = e.getYOnScreen() - resizeStartY;

                    int newWidth = Math.max(100, startSize.width + dx); // 最小宽度
                    int newHeight = Math.max(80, startSize.height + dy); // 最小高度

                    // 设置当前面板大小
                    setSize(newWidth, newHeight);
                    setPreferredSize(new Dimension(newWidth, newHeight));

                    // 调整图表面板大小
                    if (chartPanel != null) {
                        int chartWidth = newWidth - 20;
                        int chartHeight = newHeight - 20;
                        chartPanel.setPreferredSize(new Dimension(chartWidth, chartHeight));
                        chartPanel.setSize(chartWidth, chartHeight);
                    }

                    // 通知布局管理器更新
                    revalidate();
                    repaint();
                } else if (isDragging) {
                    // 处理拖动
                    int dx = e.getXOnScreen() - dragStartX;
                    int dy = e.getYOnScreen() - dragStartY;

                    int newX = panelStartX + dx;
                    int newY = panelStartY + dy;

                    // 确保面板不会移出父容器
                    Container parent = getParent();
                    if (parent != null) {
                        newX = Math.max(0, Math.min(newX, parent.getWidth() - getWidth()));
                        newY = Math.max(0, Math.min(newY, parent.getHeight() - getHeight()));
                    }

                    setLocation(newX, newY);
                    repaint();
                }
            }

            @Override
            public void mouseMoved(MouseEvent e) {
                // 鼠标移动时改变光标样式
                if (isInResizeArea(e)) {
                    setCursor(Cursor.getPredefinedCursor(Cursor.SE_RESIZE_CURSOR));
                } else {
                    setCursor(Cursor.getDefaultCursor());
                }
            }
        });
    }

    private void initCloseButton() {
        closeButton = new JButton("×"); // 使用×作为关闭图标
        closeButton.setForeground(Color.RED);
        closeButton.setFont(new Font("Arial", Font.BOLD, 16));
        closeButton.setBorderPainted(false);
        closeButton.setFocusPainted(false);
        closeButton.setBackground(new Color(0, 0, 0, 0)); // 透明背景
        closeButton.setCursor(Cursor.getPredefinedCursor(Cursor.HAND_CURSOR));

        // 设置按钮大小和位置（右上角）
        closeButton.setSize(20, 20);

        // 添加关闭事件
        closeButton.addActionListener(e -> {
            // 关闭当前面板的逻辑，例如从父容器中移除
            Container parent = getParent();
            if (parent != null) {
                parent.remove(this);
                parent.revalidate();
                parent.repaint();
            }
            this.timer.stop();
            this.timer = null;

        });

        add(closeButton);
    }

    // 重写布局方法，确保关闭按钮始终在右上角
    @Override
    public void doLayout() {
        super.doLayout();
        if (closeButton != null) {
            // 调整关闭按钮位置到右上角（留出边框间距）
            int x = getWidth() - closeButton.getWidth() - 5;
            int y = 5; // 标题栏下方5px
            closeButton.setLocation(x, y);
        }

        // 调整图表面板位置（避免被关闭按钮遮挡）
        if (chartPanel != null) {
            int chartX = 10;
            int chartY = 30; // 留出标题和关闭按钮的空间
            int chartWidth = getWidth() - 20;
            int chartHeight = getHeight() - 40;
            chartPanel.setBounds(chartX, chartY, chartWidth, chartHeight);
        }
    }

    public void createChart(TimeSeries ds) {
        dataSeries = new TimeSeries("实时数据");
        addData(ds,true);
        dataSeries.setDescription("实时数据");

        TimeSeriesCollection dataset;
        if(!isDownSampling){
            log.info("图表运行在完全模式-----------------------------------");
            dataset = new TimeSeriesCollection(dataSeries);
            log.info("绘制的点数为" + dataSeries.getItemCount());
        }else{
            log.info("图表运行在稀松模式-----------------------------------");
            TimeSeries downsampledSeries = downsampleByTimeWindow(dataSeries, 5);
            dataset = new TimeSeriesCollection(downsampledSeries);
            log.info("绘制的点数为" + downsampledSeries.getItemCount());
        }

        // 关键改进1：如果已有图表，尝试直接更新数据集（推荐）
        XYLineAndShapeRenderer renderer;
        if (chart != null) {
            // 添加新数据集
            chart.getXYPlot().setDataset(dataset);
            renderer = (XYLineAndShapeRenderer) chart.getXYPlot().getRenderer();
            chart.fireChartChanged(); // 通知图表数据变化
        } else {
            // 首次创建图表
            chart = createChart(dataset);
            chart.fireChartChanged();
            renderer = (XYLineAndShapeRenderer) chart.getXYPlot().getRenderer();
        }
        // 关键改进：确保样式设置在渲染器关联到图表之后
        if(isDownSampling){
            // 稀松模式：只显示点，不显示线
            renderer.setSeriesLinesVisible(0, false);
            renderer.setSeriesShapesVisible(0, true);
            // 增大点的尺寸
            renderer.setSeriesShape(0, new Ellipse2D.Double(-3, -3, 6, 6)); // 更大的点
            renderer.setSeriesShapesFilled(0, true); // 确保点是填充的
        }else{
            // 完全模式：只显示线，不显示点（可选）
            renderer.setSeriesStroke(0, new BasicStroke(1.2f));
            renderer.setSeriesLinesVisible(0, true);
            renderer.setSeriesShapesVisible(0, false);
        }

        chart.fireChartChanged();

        // 关键改进2：处理面板更新
        if (chartPanel != null) {
            // 移除旧面板
            remove(chartPanel);
        }
        // 创建或更新面板
        chartPanel = new ChartPanel(chart);
        chartPanel.setDomainZoomable(true);
        chartPanel.setRangeZoomable(true);
        chartPanel.setMouseWheelEnabled(true);
        chartPanel.setZoomAroundAnchor(true);
        chartPanel.setPreferredSize(new Dimension(getWidth() - 20, getHeight() - 20));

        // 添加新面板
        add(chartPanel, BorderLayout.CENTER);

        // 关键改进3：强制UI重绘
        revalidate(); // 重新计算布局
        repaint();    // 重绘组件
    }


    /**
     * 使用滑动时间窗口对TimeSeries进行稀松处理
     * @param original 原始时间序列
     * @param windowSizeSeconds 滑动窗口大小（秒）
     * @return 稀松后的时间序列
     */
    private TimeSeries downsampleByTimeWindow(TimeSeries original, int windowSizeSeconds) {
        TimeSeries result = new TimeSeries("稀松模式");

        if (original.getItemCount() == 0) {
            return result;
        }
        for(int i = 0; i < original.getItemCount(); i++){
            TimeSeriesDataItem item = original.getDataItem(i);
            if(result.getItemCount() == 0){
                result.add(item);
            }
            if(item.getValue().doubleValue() == result.getDataItem(result.getItemCount() - 1).getValue().doubleValue()){
                continue;
            }
            result.add(item);
        }
        return result;

    }


    // 检查鼠标是否在可调整大小的边界区域
    private boolean isInResizeArea(MouseEvent e) {
        Rectangle bounds = getBounds();
        return e.getX() >= bounds.width - RESIZE_BORDER &&
                e.getY() >= bounds.height - RESIZE_BORDER;
    }

    private JFreeChart createChart(TimeSeriesCollection dataset) {
        JFreeChart chart = ChartFactory.createTimeSeriesChart(
                title,
                "时间（时:分:秒.毫秒）",
                "数值",
                dataset,
                true, true, false
        );

        // 设置中文字体（避免乱码）
        Font chineseFont = new Font("SimHei", Font.PLAIN, 12);
        chart.getTitle().setFont(new Font("SimHei", Font.BOLD, 16));
        chart.getLegend().setItemFont(chineseFont);

        XYPlot plot = chart.getXYPlot();

        // 时间轴设置
        DateAxis xAxis = (DateAxis) plot.getDomainAxis();
        xAxis.setDateFormatOverride(new SimpleDateFormat("HH:mm:ss.SSS"));
        xAxis.setLabelFont(chineseFont);
        xAxis.setTickLabelFont(chineseFont);

        // Y轴设置
        NumberAxis yAxis = (NumberAxis) plot.getRangeAxis();
        yAxis.setLabelFont(chineseFont);
        yAxis.setTickLabelFont(chineseFont);


        // 曲线样式
        XYLineAndShapeRenderer renderer = new XYLineAndShapeRenderer(true, false);
        this.renderer = renderer;
        renderer.setSeriesPaint(0, Color.BLUE);
        renderer.setSeriesStroke(0, new BasicStroke(1.2f));
        plot.setRenderer(renderer);

        return chart;
    }

    // 模拟实时数据（每200ms添加一个点）
    private void startDataGenerator() {
        Random random = new Random();
        timer = new Timer(200, e -> {
            TimeSeriesDataItem dataItem = dataSeries.getDataItem(dataSeries.getItemCount() - 1);
            long lastMillisecond = dataItem.getPeriod().getFirstMillisecond() +500;
            Millisecond currentTime = new Millisecond(new Date(lastMillisecond));
            double value = 1 + random.nextGaussian() * 10; // 随机波动数据
            dataSeries.add(currentTime, value);
            System.out.println("添加数据：" + currentTime + " - " + value);

        });
        timer.start();
    }
    public TimeSeriesDataItem getFakePoint() {
        Random random = new Random();
        TimeSeriesDataItem dataItem = dataSeries.getDataItem(dataSeries.getItemCount() - 1);
        long lastMillisecond = dataItem.getPeriod().getFirstMillisecond() +500;
        Millisecond currentTime = new Millisecond(new Date(lastMillisecond));
        double value = 1 + random.nextGaussian() * 10; // 随机波动数据
        System.out.println("添加数据：" + currentTime + " - " + value);
        return new TimeSeriesDataItem(currentTime,value);
    }

    private void draw(TimeSeries timeSeries) {
        AtomicInteger counter = new AtomicInteger();
        for (int i = 0; i < timeSeries.getItemCount(); i++) {
            TimeSeriesDataItem dataItem = dataSeries.getDataItem(counter.getAndIncrement());
            double value = dataItem.getValue().doubleValue();
            dataSeries.add(dataItem.getPeriod(), value);
            System.out.println("添加数据：" + dataItem.getPeriod() + " - " + value);
        }
    }

    public void showValue(Boolean isShow){
        XYPlot xyPlot = chart.getXYPlot();
        // 如果渲染器不存在才创建新的，否则复用已有渲染器
        if (renderer == null) {
            renderer = new XYLineAndShapeRenderer();
            // 可以在这里设置一次颜色，避免默认随机色
            renderer.setSeriesPaint(0, Color.BLUE); // 为第一个系列设置固定颜色
            xyPlot.setRenderer(renderer);
        }

        // 初始设置：显示数据点但不显示标签
        renderer.setDefaultShapesVisible(isShow);
        renderer.setDefaultItemLabelGenerator(new StandardXYItemLabelGenerator());
        renderer.setDefaultItemLabelsVisible(isShow); // 初始隐藏

        xyPlot.setRenderer(renderer);
        chart.fireChartChanged();
        isShowValue = isShow;

    }

    public boolean chartIsShowValue(){
        return  isShowValue;
    }

    // 单独设置颜色的方法（可按需调用）
    public void setSeriesColor(int seriesIndex, Color color) {
        if (renderer != null && seriesIndex >= 0) {
            renderer.setSeriesPaint(seriesIndex, color);
            chart.fireChartChanged(); // 刷新图表
        }
    }

    // 向DataSeries添加数据
    public void addData(TimeSeries dataItem,boolean isInit) {
        if (dataSeries == null || dataItem == null || dataItem.getItemCount() == 0) {
            return;
        }
        for (int i = 0; i < dataItem.getItemCount(); i++) {
            TimeSeriesDataItem item = dataItem.getDataItem(i);
            dataSeries.addOrUpdate(item.getPeriod(), item.getValue());
        }

        // 如果点的数据太多，大于3000 ，丢弃前面的数据
        if(dataSeries.getItemCount()>3000 && !isInit){
            dataSeries.delete(0,dataItem.getItemCount()-1);
        }

    }

}
