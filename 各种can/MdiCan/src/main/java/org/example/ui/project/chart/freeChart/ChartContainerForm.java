/*
 * Created by JFormDesigner on Thu Sep 18 18:57:45 CST 2025
 */

package org.example.ui.project.chart.freeChart;

import lombok.extern.slf4j.Slf4j;
import org.apache.commons.collections4.ListUtils;
import org.example.pojo.data.CanMessage;
import org.example.pojo.data.ChartInfoVo;
import org.example.pojo.dbc.DbcSignal;
import org.example.pojo.project.SignalInGroup;
import org.example.ui.project.chart.group.GroupMangeView;
import org.example.ui.project.chart.group.SignalSelectionListener;
import org.example.ui.project.waitting.WaittingView;
import org.example.utils.StringUtil;
import org.example.utils.chart.TimeSeriesCompletor;
import org.example.utils.web.ChartDataCacher;
import org.jfree.data.time.RegularTimePeriod;
import org.jfree.data.time.TimeSeries;
import org.jfree.data.time.TimeSeriesDataItem;
import org.springframework.beans.BeanUtils;

import java.awt.*;
import java.awt.event.*;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import javax.swing.*;

/**
 * @author jingq
 */
@Slf4j
public class ChartContainerForm extends JFrame {
    private List<DraggableTimeChart> charts = new ArrayList<>();

    private GroupMangeView groupMangeView;

    private SignalSelectionListener signalSelectionListener;

    private Map<String, ChartInfoVo> chartMap = new HashMap<>();

    private Timer timer;

    private boolean isDownSampling = false;

    private boolean isDrawing = true;

    private boolean isOneTime = false;

    public ChartContainerForm() {
        initComponents();
        setExtendedState(JFrame.MAXIMIZED_BOTH);
        setupCloseEvent();
        panel.setViewportView(mainPanel);
        signalSelectionListener = new SignalSelectionListener() {
            @Override
            public void onSignalsSelected(List<SignalInGroup> signals) {
                WaittingView waittingView = new WaittingView(ChartContainerForm.this);
                waittingView.show();
                // 选择绑定数据的项目
                ProjectSelecter projectSelecter = new ProjectSelecter(ChartContainerForm.this);
                projectSelecter.setVisible(true);
                if(!projectSelecter.getSelectedItem()){
                    return;
                }
                Long selectedProjectId = projectSelecter.getSelectedProjectId();
                List<TimeSeries> timeSeries = new ArrayList<>();
                // 转换为DbcSignal列表
                for (SignalInGroup signal : signals) {
                    DbcSignal dbcSignal = new DbcSignal();
                    BeanUtils.copyProperties(signal, dbcSignal);

                    SignalValueExtractor extractor = new SignalValueExtractor();
                    extractor.init(selectedProjectId,dbcSignal);
                    TimeSeries dataFromDb = extractor.getDataFromDb(0.0);
//                    timeSeries.add(dataFromDb);
                    // 添加图表
                    DraggableTimeChart chart = new DraggableTimeChart("   ", new Color(255, 220, 220));
                    addChartPanel(chart);
                    chart.setTitle(signal.getMessageId() + ": " +signal.getName());
                    chart.createChart(dataFromDb);
                    chart.showValue(false);
                    // 存信息
                    ChartInfoVo vo = new ChartInfoVo();
                    vo.setCanMessageId(signal.getMessageId());
                    vo.setSignalName(signal.getName());
                    vo.setMaxIndex(extractor.getMaxCounter());
                    vo.setProjectId(selectedProjectId);
                    vo.setExtractor(extractor);
                    vo.setChart(chart);
                    // put to current drawing map
                    ChartDataCacher.getInstance().setChartToCurrent(selectedProjectId, signal.getMessageId());
                    chartMap.put(signal.getMessageId() + ":" +signal.getName() + ":" + selectedProjectId,vo);
                }
                if(isOneTime){
                    List<RegularTimePeriod> allPoints = TimeSeriesCompletor.collectTimePoints(timeSeries);
                    for(String key : chartMap.keySet()){
                        DraggableTimeChart chart = chartMap.get(key).getChart();
                        TimeSeries newDataSeries = TimeSeriesCompletor.completeTimeSeries(chart.getDataSeries(), allPoints);
                        chart.setDataSeries(newDataSeries);
                    }
                }

                waittingView.close();

                // 开启定时作业更新图表数据
                 startTimer();
            }
        };
    }

    private void startTimer() {
        log.info("开始定时更新图表数据");
        timer = new Timer(300, new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                try{
                    List<TimeSeries> timeSeries = new ArrayList<>();
                    for(String key : chartMap.keySet()){
                        ChartInfoVo chartInfoVo = chartMap.get(key);
                        Long maxCounter = chartInfoVo.getExtractor().getMaxCounter();
                        Double maxTime = chartInfoVo.getExtractor().getMaxTime();
                        log.info("获取数据：" + key + " - maxTime = " + maxTime);
                        if(maxTime == null){
                            maxTime = 0.0;
                        }
                        // 需要先清理一下历史数据
                        chartInfoVo.getExtractor().cleanDataSeries();
                        // 这是从数据库获取
                        //TimeSeries dataFromDb = chartInfoVo.getExtractor().getDataFromDb(maxTime);
                        // 这是从缓存获取
                        TimeSeries dataFromDb = chartInfoVo.getExtractor().getDataFromCatcher();
                        log.info("获取数据：" + key + " - dataFromDb.size() = " + dataFromDb.getItemCount());
                        chartInfoVo.getChart().addData(dataFromDb,false);
                        timeSeries.add(chartInfoVo.getChart().getDataSeries());
                    }
                    List<RegularTimePeriod> allPoints = TimeSeriesCompletor.collectTimePoints(timeSeries);
                    for (String key : chartMap.keySet()){
                        ChartInfoVo chartInfoVo = chartMap.get(key);
                        chartInfoVo.getExtractor().cleanDataFromCatch();
                        if(isOneTime){
                            DraggableTimeChart chart = chartInfoVo.getChart();
                            TimeSeries newSerial = TimeSeriesCompletor.completeTimeSeries(chart.getDataSeries(), allPoints);
                            chart.setDataSeries(newSerial);
                        }
                    }

                }catch (Exception ex){
                    ex.printStackTrace();
                    log.info("update chart error");
                }
            }
        });
        timer.start();
    }

    private void setupCloseEvent() {
        // 设置默认关闭操作为"不做任何操作"，交给我们的监听器处理
        setDefaultCloseOperation(JFrame.DO_NOTHING_ON_CLOSE);

        // 添加窗口监听器，重写关闭事件
        addWindowListener(new WindowAdapter() {
            @Override
            public void windowClosing(WindowEvent e) {
                // 1. 弹出确认对话框
                int result = JOptionPane.showConfirmDialog(
                        ChartContainerForm.this,
                        "确定要关闭窗口吗？未保存的数据可能会丢失。",
                        "关闭确认",
                        JOptionPane.YES_NO_OPTION,
                        JOptionPane.WARNING_MESSAGE
                );

                // 2. 根据用户选择执行操作
                if (result == JOptionPane.YES_OPTION) {
                    // 3. 执行关闭前的清理操作
                    cleanupResources();

                    // 4. 真正关闭窗口
                    dispose();
                    // 如果是应用程序最后一个窗口，可退出JVM
//                    System.exit(0);
                }
                // 用户选择"否"，则不执行任何操作，窗口保持打开
            }
        });
    }

    // 关闭前的资源清理操作
    private void cleanupResources() {
        // 示例：释放图表资源
        if (this.timer != null) {
            // 假设ChartColorManager有释放资源的方法
            this.timer.stop();
            this.timer = null;
            System.out.println("资源已释放");
        }

        // 可以添加其他清理逻辑，如保存配置、关闭数据库连接等
        charts = null;
        chartMap = null;
    }

    public void addChartPanel(DraggableTimeChart chartPanel) {
        Point point = getChartPoint();
        chartPanel.setLocation(point);
        chartPanel.setSize(getWidth()-50,400);
        mainPanel.add(chartPanel);
        resetMainPanelSize();
        mainPanel.revalidate();
        mainPanel.repaint();
        charts.add(chartPanel);
    }

    private void resetMainPanelSize() {
        int h = 0;
        for (DraggableTimeChart chart : charts) {
            h += chart.getHeight();
        }
        if(h<500){
            h = 500;
        }
        mainPanel.setPreferredSize(new Dimension(getWidth() -20, h + 20));
    }


    // 计算应该把图放在哪个位置
    private Point getChartPoint() {
        if(charts.isEmpty()){
            return new Point(10,10);
        }

        DraggableTimeChart draggableTimeChart = charts.get(charts.size() - 1);
        // 获取最后一个图表的位置
        Point lastChartPoint = draggableTimeChart.getLocation();
        Point point = new Point(10, (int) lastChartPoint.getY() + draggableTimeChart.getHeight() + 10);
        return point;
    }

    private void openSignelGroupView(ActionEvent e) {
        // TODO add your code here
        groupMangeView = new GroupMangeView(true);
        groupMangeView.setProviderMode(true);
        groupMangeView.setSignalSelectionListener(signalSelectionListener);
        groupMangeView.setVisible(true);
    }

    private void tragelShowValue(ActionEvent e) {
        // TODO add your code here
        // 切换是否显示值.需要选择要设置哪个图
        // 循环chartMap
        for(String key : chartMap.keySet()){
            ChartInfoVo chartInfoVo = chartMap.get(key);
            chartInfoVo.getChart().showValue(!chartInfoVo.getChart().isShowValue());
        }

    }

    private void selectChartColor(ActionEvent e) {
        // TODO add your code here
        // 显示颜色选择对话框
        Color selectedColor = JColorChooser.showDialog(
                this,
                "选择曲线的颜色",
                Color.BLUE
        );
        // 如果用户选择了颜色（没有点击取消）
        if (selectedColor != null) {
            setChartColor(selectedColor);
        }
    }

    private void setChartColor(Color selectedColor) {
        for(String key : chartMap.keySet()){
            ChartInfoVo chartInfoVo = chartMap.get(key);
            chartInfoVo.getChart().setSeriesColor(0,selectedColor);
        }
    }

    private void toggleButton1(ActionEvent e) {
        // TODO add your code here
        isDownSampling = !isDownSampling;
        if(!isDownSampling){
            toggleButton1.setText("完全模式");
        }else{
            toggleButton1.setText("稀松模式");
        }
        for(String key : chartMap.keySet()){
            ChartInfoVo chartInfoVo = chartMap.get(key);
            chartInfoVo.getChart().setDownSampling(isDownSampling);
            chartInfoVo.getChart().createChart(chartInfoVo.getChart().getDataSeries());
        }
    }

    private void startOrStopDraw(ActionEvent e) {
        // TODO add your code here
        isDrawing = !isDrawing;
        if(isDrawing){
            button4.setText("停止");
            if(timer!=null && !timer.isRunning()){
                timer.start();
            }
        }else{
            button4.setText("开始");
            if(timer !=null && timer.isRunning()){
                timer.stop();
            }
        }
    }

    // 拉齐时间轴
    private void toOneTime(ActionEvent e) {
        // TODO add your code here
        isOneTime = !isOneTime;
        if(isOneTime){
            button5.setText("默认时间轴");
        }else{
            button5.setText("拉齐时间轴");
        }
    }


    private void initComponents() {
        // JFormDesigner - Component initialization - DO NOT MODIFY  //GEN-BEGIN:initComponents  @formatter:off
        panel2 = new JPanel();
        toolBar1 = new JToolBar();
        button1 = new JButton();
        button2 = new JButton();
        button3 = new JButton();
        toggleButton1 = new JToggleButton();
        button4 = new JButton();
        button5 = new JButton();
        panel = new JScrollPane();
        mainPanel = new JPanel();

        //======== this ========
        Container contentPane = getContentPane();
        contentPane.setLayout(new BorderLayout());

        //======== panel2 ========
        {
            panel2.setLayout(new BoxLayout(panel2, BoxLayout.X_AXIS));

            //======== toolBar1 ========
            {

                //---- button1 ----
                button1.setText("\u4fe1\u53f7\u7ec4");
                button1.addActionListener(e -> openSignelGroupView(e));
                toolBar1.add(button1);

                //---- button2 ----
                button2.setText("\u663e\u793a\u503c");
                button2.addActionListener(e -> tragelShowValue(e));
                toolBar1.add(button2);

                //---- button3 ----
                button3.setText("\u66f2\u7ebf\u989c\u8272");
                button3.addActionListener(e -> selectChartColor(e));
                toolBar1.add(button3);
            }
            panel2.add(toolBar1);

            //---- toggleButton1 ----
            toggleButton1.setText("\u5b8c\u5168\u6a21\u5f0f");
            toggleButton1.addActionListener(e -> toggleButton1(e));
            panel2.add(toggleButton1);

            //---- button4 ----
            button4.setText("\u505c\u6b62");
            button4.addActionListener(e -> startOrStopDraw(e));
            panel2.add(button4);

            //---- button5 ----
            button5.setText("\u62c9\u9f50\u65f6\u95f4\u8f74");
            button5.addActionListener(e -> toOneTime(e));
            panel2.add(button5);
        }
        contentPane.add(panel2, BorderLayout.PAGE_START);

        //======== panel ========
        {

            //======== mainPanel ========
            {
                mainPanel.setLayout(new BoxLayout(mainPanel, BoxLayout.Y_AXIS));
            }
            panel.setViewportView(mainPanel);
        }
        contentPane.add(panel, BorderLayout.CENTER);
        setSize(755, 450);
        setLocationRelativeTo(getOwner());
        // JFormDesigner - End of component initialization  //GEN-END:initComponents  @formatter:on
    }

    // JFormDesigner - Variables declaration - DO NOT MODIFY  //GEN-BEGIN:variables  @formatter:off
    private JPanel panel2;
    private JToolBar toolBar1;
    private JButton button1;
    private JButton button2;
    private JButton button3;
    private JToggleButton toggleButton1;
    private JButton button4;
    private JButton button5;
    private JScrollPane panel;
    private JPanel mainPanel;
    // JFormDesigner - End of variables declaration  //GEN-END:variables  @formatter:on

    public static void main(String[] args) {
        ChartContainerForm chartContainerForm = new ChartContainerForm();
        chartContainerForm.setVisible(true);

    }
}
