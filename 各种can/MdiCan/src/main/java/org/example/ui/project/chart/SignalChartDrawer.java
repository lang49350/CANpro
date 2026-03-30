package org.example.ui.project.chart;

import org.apache.commons.collections4.ListUtils;
import org.example.pojo.data.CanMessage;
import org.example.pojo.dbc.DbcMessage;
import org.example.pojo.dbc.DbcSignal;
import org.example.pojo.project.SignalInGroup;
import org.example.ui.project.chart.group.GroupMangeView;
import org.example.ui.project.chart.group.SignalSelectionListener;
import org.example.utils.chart.TimeFormater;
import org.example.utils.db.ExplanCanData;
import org.springframework.beans.BeanUtils;

import javax.swing.*;
import javax.swing.Timer;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;
import java.awt.*;
import java.awt.event.*;
import java.text.DecimalFormat;
import java.util.*;
import java.util.List;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.TimeUnit;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.stream.Collectors;

// 信号曲线绘制工具类
public class SignalChartDrawer extends JFrame {

    public static List<SignalChartDrawer> instances = new ArrayList<>();

    private List<CanMessage> canMessages;
    private List<DbcSignal> allSignals;
    private Map<String, List<Double>> signalValues;
    private Map<String, List<Double>> signalTimestamps; // 存储每个信号对应的时间戳
    private Map<String, Boolean> signalVisibility;
    private Map<String, Color> signalColors;
    private Map<String, Float> signalStrokeWidths;
    private Map<String, Boolean> signalValueVisibility;
    private JList<String> signalList;
    private DefaultListModel<String> signalListModel;
    private JScrollPane signalListScrollPane;
    private ChartPanel chartPanel;
    private JScrollPane chartScrollPane;
    private JSplitPane splitPane;
    private JPanel controlPanel;
    private JComboBox<String> signalSelector;
    private JCheckBox showValueCheckBox;
    private JButton bgColorButton;
    private JButton curveColorButton;
    private JSlider strokeWidthSlider;
    private JLabel strokeWidthLabel;

    private SignalSelectionListener signalSelectionListener;

    private GroupMangeView groupMangeView;

    // 缩放相关参数
    private double scaleX = 1.0;
    private double scaleY = 1.0;
    private double offsetX = 0.0;
    private double offsetY = 0.0;
    private static final double ZOOM_FACTOR = 1.1;

    // 每个信号Y轴的边距
    private static final int Y_AXIS_WIDTH = 60;
    // 信号区域之间的间距
    private static final int SIGNAL_SPACING = 20;

    // 最早和最晚的时间戳
    private double earliestTimestamp = Double.MAX_VALUE;
    private double latestTimestamp = Double.MIN_VALUE;

    private boolean update = true;

    // 新增状态变量记录用户交互
    private boolean userIsDragging = false; // 用户是否正在拖拽
    private boolean autoScrollPaused = false; // 自动滚动是否被暂停
    private Timer autoScrollResumeTimer; // 用于恢复自动滚动的计时器

    // 新增动画相关变量
    private boolean isAnimating = false; // 是否正在执行动画
    private double targetOffsetX; // 动画目标偏移量
    private double animationStep; // 每帧动画的步长
    private Timer animationTimer; // 动画定时器
    private static final int ANIMATION_DURATION = 300; // 动画持续时间(毫秒)
    private static final int FRAME_DELAY = 15; // 每帧间隔(毫秒)，约60fps

    // 新增播放相关变量
    private boolean isPlaying = false; // 是否正在播放
    private double playbackStartTime; // 播放开始时间(毫秒)
    private double playbackTotalDuration; // 播放总时长(毫秒)
    private double playbackStartOffset; // 播放起始偏移量
    private double playbackEndOffset; // 播放结束偏移量
    private Timer playbackTimer; // 播放定时器

    private JSpinner spinner;

    private JRadioButton isAutoScroll;


    public SignalChartDrawer(List<CanMessage> canMessages, List<DbcSignal> dbcSignals) {
        // 最大化
        setExtendedState(JFrame.MAXIMIZED_BOTH);
        setResizable(true);
        this.canMessages = canMessages;
        this.allSignals = dbcSignals;
        this.signalValues = new ConcurrentHashMap<>();
        this.signalTimestamps = new ConcurrentHashMap<>(); // 初始化时间戳映射
        this.signalVisibility = new ConcurrentHashMap<>();
        this.signalColors = new ConcurrentHashMap<>();
        this.signalStrokeWidths = new ConcurrentHashMap<>();
        this.signalValueVisibility = new ConcurrentHashMap<>();

        initComponents();
        setupLayout();
        setupListeners();
        extractSignalValues();
        // 初始化计时器（在构造方法或初始化方法中）
        autoScrollResumeTimer = new Timer(1000, e -> {
            autoScrollPaused = false; // 3秒后恢复自动滚动
        });
        autoScrollResumeTimer.setRepeats(false); // 只执行一次
        // 3. 设置拖拽事件监听器（关键步骤）
        setupDragListeners();
        // 初始化动画定时器
        animationTimer = new Timer(FRAME_DELAY, e -> {
            updateAnimationFrame();
        });
        animationTimer.setRepeats(true);

        // 初始化播放定时器
        playbackTimer = new Timer(15, e -> {
            updatePlaybackPosition();
        });
        playbackTimer.setRepeats(true);
        // 加入到实体清单
        addInstances(this);

    }

    public void addInstances(SignalChartDrawer instance){
        instances.add(instance);
    }

    public void startSmartPlayback() {
        // 先计算信号总时长
        long totalMs = calculateSelectedSignalDurationMs();
        if (totalMs <= 0) {
            System.out.println("信号数据不足，无法启动回访");
            return;
        }

        // 根据信号实际时长自动调整播放速度
        // 例如：最短播放时间5秒，最长30秒，避免过快或过慢
        long playbackDurationMs = Math.max(5000, totalMs);

        System.out.println("信号实际时长: " + totalMs + "ms，回访时长: " + playbackDurationMs + "ms");
        // 根据用户选择的回放速度缩放时间
        double value = (double) spinner.getValue();
        startPlayback(playbackDurationMs/value); // 调用之前实现的播放方法
    }

    /**
     * 开始播放：从起始位置滚动到最后位置
     * @param duration 滚动总时间(毫秒)
     */
    public void startPlayback(double duration) {
        // 停止可能正在进行的动画或播放
        if (isAnimating) {
            animationTimer.stop();
            isAnimating = false;
        }
        if (isPlaying) {
            playbackTimer.stop();
        }

        // 计算播放的起始和结束偏移量
        playbackStartOffset = offsetX; // 起始位置
        double visibleTimeWidth = chartPanel.getWidth() / scaleX;
        playbackEndOffset = Math.max(0, latestTimestamp - earliestTimestamp - visibleTimeWidth * 0.8);

        // 保存播放参数
        playbackTotalDuration = duration;
        playbackStartTime = System.currentTimeMillis();
        isPlaying = true;

        // 暂停自动滚动
        autoScrollPaused = true;

        // 启动播放定时器
        playbackTimer.start();
    }

    /**
     * 停止播放
     */
    public void stopPlayback() {
        if (isPlaying) {
            isPlaying = false;
            playbackTimer.stop();
            // 恢复自动滚动
            autoScrollPaused = false;
        }
    }

    /**
     * 更新播放位置（每帧调用）
     */
    private void updatePlaybackPosition() {
        // 计算已播放时间比例
        double elapsed = System.currentTimeMillis() - playbackStartTime;
        double progress = Math.min(elapsed / playbackTotalDuration, 1.0);

        // 计算当前应该的偏移量
        offsetX = playbackStartOffset + (playbackEndOffset - playbackStartOffset) * progress;
        chartPanel.repaint();

        // 播放结束
        if (progress >= 1.0) {
            stopPlayback();
        }
    }

    // 动画帧更新逻辑
    private void updateAnimationFrame() {
        // 计算当前偏移量与目标偏移量的差距
        double diff = targetOffsetX - offsetX;

        // 当差距小于步长时，直接到达目标值并停止动画
        if (Math.abs(diff) < Math.abs(animationStep)) {
            offsetX = targetOffsetX;
            isAnimating = false;
            animationTimer.stop();
            chartPanel.repaint();
            return;
        }

        // 逐步逼近目标值
        offsetX += animationStep;
        chartPanel.repaint();
    }

    // 启动动画的方法（替代直接设置offsetX）
    private void startScrollAnimation(double newTargetOffset) {
        // 如果正在动画中，先停止
        if (isAnimating) {
            animationTimer.stop();
        }

        // 设置动画目标和步长
        targetOffsetX = newTargetOffset;
        double distance = targetOffsetX - offsetX;
        int totalFrames = ANIMATION_DURATION / FRAME_DELAY; // 总帧数
        animationStep = distance / totalFrames; // 每帧移动的距离

        // 启动动画
        isAnimating = true;
        animationTimer.start();
    }

    public void refreshChart(List<CanMessage> data) {
        System.out.println("更新图表任务执行...");
        if(update){
            canMessages.clear();
            canMessages.addAll(data);
            extractSignalValues();
            // 新数据处理后自动滚动到最新位置
            autoScrollToLatest();
        }

    }

    // 添加自动滚动方法
    private void autoScrollToLatest() {
        // 数据有效性检查
        if (latestTimestamp <= earliestTimestamp || chartPanel.getWidth() <= 0
                || userIsDragging || autoScrollPaused || isPlaying) {
            // 用户操作时不执行自动滚动
            return;
        }
        if (latestTimestamp > earliestTimestamp && chartPanel.getWidth() > 0) {
            // 计算实际数据总时长
            double totalDataDuration = latestTimestamp - earliestTimestamp;
            // 计算可视区域宽度（根据当前缩放比例）
            double visibleWidth = chartPanel.getWidth() / scaleX;
            // 大数据量适配：当数据总量超过可视区域2倍时，才启用自动滚动到最新
            // 避免小数据量时过度滚动导致只看得到末尾
            boolean needAutoScroll = totalDataDuration > visibleWidth * 2
                    || latestTimestamp > (earliestTimestamp + offsetX + visibleWidth);
            if (needAutoScroll) {
                // 计算偏移量时保留一定比例的历史数据可见（如20%）
                double historyRatio = 0.2; // 保留20%的历史数据在左侧
                double newOffset = latestTimestamp - earliestTimestamp
                        - visibleWidth * (1 - historyRatio);

                // 确保偏移量不会小于0（避免显示负时间区域）
                offsetX = Math.max(0, newOffset);
                newOffset = Math.max(0, newOffset);
                //chartPanel.repaint();
                // 使用动画过渡到新偏移量（替代直接赋值）
                startScrollAnimation(newOffset);
            } else if (offsetX < 0) {
                // 修正偏移量为非负值
                offsetX = 0;
                chartPanel.repaint();
            }
        }
    }

    // 添加处理用户拖拽的方法（需要绑定到鼠标事件）
    private void setupDragListeners() {
        // 鼠标按下时标记为拖拽状态
        chartPanel.addMouseListener(new MouseAdapter() {
            @Override
            public void mousePressed(MouseEvent e) {
                userIsDragging = true;
                autoScrollPaused = true; // 暂停自动滚动
                autoScrollResumeTimer.stop(); // 停止计时器
                // 拖拽开始时停止动画
                if (isAnimating) {
                    isAnimating = false;
                    animationTimer.stop();
                }
                if (isPlaying) {
                    stopPlayback();
                }
            }

            @Override
            public void mouseReleased(MouseEvent e) {
                userIsDragging = false;
                // 启动计时器，3秒后恢复自动滚动
                // 检查控件状态,只有控件被选中状态才允许自动恢复滚动
                if(isAutoScroll.isSelected()){
                    autoScrollResumeTimer.start();
                }
            }
        });

        // 鼠标拖拽时更新偏移量（原有拖拽逻辑）
        chartPanel.addMouseMotionListener(new MouseMotionAdapter() {
            @Override
            public void mouseDragged(MouseEvent e) {
                // 原有拖拽计算逻辑...
                // 例如：offsetX = ...;
                chartPanel.repaint();
            }
        });
    }

    // 获取当前选中信号的所有时间戳（毫秒级）
    public List<Long> getSelectedSignalTimestampsMs() {
        String selectedSignal = (String) signalSelector.getSelectedItem();
        if (selectedSignal == null || !signalTimestamps.containsKey(selectedSignal)) {
            return new ArrayList<>();
        }

        // 转换为Long类型的毫秒级时间戳列表
        List<Long> timestampsMs = new ArrayList<>();
        for (double ts : signalTimestamps.get(selectedSignal)) {
            // 假设原始时间戳是double类型的毫秒数，转换为long
            timestampsMs.add((long) ts);
        }
        return timestampsMs;
    }

    // 计算选中信号的时长（毫秒）并更新状态变量
    public long calculateSelectedSignalDurationMs() {
        List<Long> timestampsMs = getSelectedSignalTimestampsMs();
        if (timestampsMs.size() < 2) {
            return 0;
        }

        // 排序时间戳（确保准确性）
        Collections.sort(timestampsMs);

        // 记录起始和结束时间戳（毫秒）
        long selectedSignalStartMs = timestampsMs.get(0);
        long selectedSignalEndMs = timestampsMs.get(timestampsMs.size() - 1);

        // 计算总时长（毫秒）
        return selectedSignalEndMs - selectedSignalStartMs;
    }

    public SignalChartDrawer() {
        setExtendedState(JFrame.MAXIMIZED_BOTH);
    }

    private void initComponents() {
        // 回调数据
        signalSelectionListener = new SignalSelectionListener() {
            @Override
            public void onSignalsSelected(List<SignalInGroup> signals) {
                // 转换为DbcSignal列表
                for (SignalInGroup signal : signals) {
                    DbcSignal dbcSignal = new DbcSignal();
                    BeanUtils.copyProperties(signal, dbcSignal);
                    // 如果当前列表中已经有这个信号就忽略
                    long count = ListUtils.emptyIfNull(allSignals).stream().filter(o -> {
                        boolean name_eq = o.getName().equals(dbcSignal.getName());
                        boolean messageId_eq = o.getMessageId().equals(dbcSignal.getMessageId());
                        return name_eq && messageId_eq;
                    }).count();
                    if(count == 0){
                        allSignals.add(dbcSignal);
                        signalListModel.addElement(signal.getName());
                        signalSelector.addItem(signal.getName());
                    }
                }
                // 默认选择第一个信号
                if (signalSelector.getItemCount() > 0) {
                    signalSelector.setSelectedIndex(0);
                }
                extractSignalValues();
            }
        };

        // 创建菜单栏
        JMenuBar menuBar = new JMenuBar();
        JMenu addMenu = new JMenu("新增");
        JMenu optionMenu = new JMenu("操作");

        JMenuItem addFromConfigGroupItem = new JMenuItem("从配置组新增");
        addFromConfigGroupItem.addActionListener(e -> {
            // 处理从配置组新增的逻辑
            System.out.println("从配置组新增");
            groupMangeView = new GroupMangeView(true);
            groupMangeView.setProviderMode(true);
            groupMangeView.setSignalSelectionListener(signalSelectionListener);
            groupMangeView.setVisible(true);
        });

        JMenuItem stopItem = new JMenuItem("停止更新");
        stopItem.addActionListener(e -> {
            // 处理停止的逻辑
            System.out.println("停止");
            this.update = false;
        });

        JMenuItem startItem = new JMenuItem("开始更新");
        startItem.addActionListener(e -> {
            // 处理开始的逻辑
            System.out.println("停止");
            this.update = true;
        });

        addMenu.add(addFromConfigGroupItem);
        optionMenu.add(stopItem);
        optionMenu.add(startItem);
        // 添加菜单
        menuBar.add(addMenu);
        menuBar.add(optionMenu);


        // 添加菜单栏
        setJMenuBar(menuBar);
        // 初始化信号列表
        signalListModel = new DefaultListModel<>();
        signalList = new JList<>(signalListModel);
        signalList.setSelectionMode(ListSelectionModel.MULTIPLE_INTERVAL_SELECTION);
        signalListScrollPane = new JScrollPane(signalList);

        // 初始化图表面板
        chartPanel = new ChartPanel();
        chartScrollPane = new JScrollPane(chartPanel);

        // 初始化控制面板
        controlPanel = new JPanel();
        controlPanel.setLayout(new BoxLayout(controlPanel, BoxLayout.Y_AXIS));

        // 信号选择下拉框
        signalSelector = new JComboBox<>();
        JPanel signalSelectorPanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
        signalSelectorPanel.add(new JLabel("选择信号:"));
        signalSelectorPanel.add(signalSelector);
        controlPanel.add(signalSelectorPanel);

        // 播放
        JButton play = new JButton("回放");
        spinner = new JSpinner();
        spinner.setModel(new SpinnerNumberModel(1, 0.5, 3, 0.5));
        JPanel playPanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
        playPanel.add(new JLabel("速度:"));
        playPanel.add(spinner);
        playPanel.add(play);
        controlPanel.add(playPanel);
        play.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                startSmartPlayback();
            }
        });
        spinner.addChangeListener(new ChangeListener() {

            @Override
            public void stateChanged(ChangeEvent e) {
                // 更新播放速度
                startSmartPlayback();
            }
        });

        // 是否显示值复选框
        showValueCheckBox = new JCheckBox("显示值");
        JPanel showValuePanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
        showValuePanel.add(showValueCheckBox);
        controlPanel.add(showValuePanel);

        // 是否自动滚动
        isAutoScroll = new JRadioButton("自动滚动");
        // 初始化
        isAutoScroll.setSelected(!autoScrollPaused);
        JPanel autoScrollPanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
        autoScrollPanel.add(isAutoScroll);
        controlPanel.add(autoScrollPanel);
        isAutoScroll.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                // 获取自动滚动状态
                // 更新自动滚动状态
                autoScrollPaused = !isAutoScroll.isSelected();
                if(autoScrollPaused){
                    if(autoScrollResumeTimer.isRunning()){
                        autoScrollResumeTimer.stop();
                    }
                }else{
                    if(!autoScrollResumeTimer.isRunning()){
                        autoScrollResumeTimer.start();
                    }
                }

            }
        });

        // 背景颜色按钮
        bgColorButton = new JButton("设置背景颜色");
        JPanel bgColorPanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
        bgColorPanel.add(bgColorButton);
        controlPanel.add(bgColorPanel);

        // 曲线颜色按钮
        curveColorButton = new JButton("设置曲线颜色");
        JPanel curveColorPanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
        curveColorPanel.add(curveColorButton);
        controlPanel.add(curveColorPanel);

        // 曲线粗细滑块
        strokeWidthSlider = new JSlider(1, 10, 2);
        strokeWidthLabel = new JLabel("曲线粗细: 2");
        JPanel strokeWidthPanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
        strokeWidthPanel.add(strokeWidthLabel);
        strokeWidthPanel.add(strokeWidthSlider);
        controlPanel.add(strokeWidthPanel);

        // 初始化分割面板
        JSplitPane leftSplitPane = new JSplitPane(JSplitPane.VERTICAL_SPLIT,
                signalListScrollPane, controlPanel);
        leftSplitPane.setDividerLocation(300);

        splitPane = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT,
                leftSplitPane, chartScrollPane);
        splitPane.setDividerLocation(250);

    }

    private void setupLayout() {
        setLayout(new BorderLayout());
        add(splitPane, BorderLayout.CENTER);
    }

    private void setupListeners() {
        // 信号列表选择变化事件
        signalList.addListSelectionListener(e -> {
            if (!e.getValueIsAdjusting()) {
                updateSignalVisibility();
            }
        });

        // 鼠标滚轮缩放事件
        chartPanel.addMouseWheelListener(e -> {
            if (e.getWheelRotation() < 0) {
                // 放大
                scaleX *= ZOOM_FACTOR;
                scaleY *= ZOOM_FACTOR;
            } else {
                // 缩小
                scaleX /= ZOOM_FACTOR;
                scaleY /= ZOOM_FACTOR;
            }
            chartPanel.repaint();
        });

        // 鼠标拖动事件
        MouseAdapter mouseAdapter = new MouseAdapter() {
            private Point lastPoint;

            @Override
            public void mousePressed(MouseEvent e) {
                lastPoint = e.getPoint();
            }

            @Override
            public void mouseDragged(MouseEvent e) {
                if (lastPoint != null) {
                    int dx = e.getX() - lastPoint.x;
                    int dy = e.getY() - lastPoint.y;

                    offsetX -= dx / scaleX;
                    offsetY -= dy / scaleY;

                    // 确保offsetX在合理范围内
                    offsetX = Math.max(0, Math.min(offsetX, latestTimestamp - earliestTimestamp));

                    lastPoint = e.getPoint();
                    chartPanel.repaint();
                }
            }

            @Override
            public void mouseReleased(MouseEvent e) {
                lastPoint = null;
            }
        };

        chartPanel.addMouseListener(mouseAdapter);
        chartPanel.addMouseMotionListener(mouseAdapter);

        // 信号选择下拉框变化事件
        signalSelector.addActionListener(e -> {
            String selectedSignal = (String) signalSelector.getSelectedItem();
            if (selectedSignal != null) {
                updateControlPanelState(selectedSignal);
            }
        });

        // 显示值复选框事件
        showValueCheckBox.addActionListener(e -> {
            String selectedSignal = (String) signalSelector.getSelectedItem();
            if (selectedSignal != null) {
                setSignalValueVisibility(selectedSignal, showValueCheckBox.isSelected());
            }
        });

        // 背景颜色按钮事件
        bgColorButton.addActionListener(e -> {
            String selectedSignal = (String) signalSelector.getSelectedItem();
            if (selectedSignal != null) {
                Color newColor = JColorChooser.showDialog(
                        this,
                        "选择背景颜色",
                        chartPanel.signalBackgroundColors.getOrDefault(selectedSignal, Color.WHITE)
                );
                if (newColor != null) {
                    setSignalBackgroundColor(selectedSignal, newColor);
                }
            }
        });

        // 曲线颜色按钮事件
        curveColorButton.addActionListener(e -> {
            String selectedSignal = (String) signalSelector.getSelectedItem();
            if (selectedSignal != null) {
                Color newColor = JColorChooser.showDialog(
                        this,
                        "选择曲线颜色",
                        signalColors.get(selectedSignal)
                );
                if (newColor != null) {
                    setSignalColor(selectedSignal, newColor);
                }
            }
        });

        // 曲线粗细滑块事件
        strokeWidthSlider.addChangeListener(e -> {
            String selectedSignal = (String) signalSelector.getSelectedItem();
            if (selectedSignal != null) {
                float width = strokeWidthSlider.getValue();
                strokeWidthLabel.setText("曲线粗细: " + width);
                setSignalStrokeWidth(selectedSignal, width);
            }
        });
    }

    // 更新控制面板状态
    private void updateControlPanelState(String signalName) {
        showValueCheckBox.setSelected(signalValueVisibility.getOrDefault(signalName, false));
        strokeWidthSlider.setValue(Math.round(signalStrokeWidths.getOrDefault(signalName, 1f)));
        strokeWidthLabel.setText("曲线粗细: " + strokeWidthSlider.getValue());
    }

    // 提取每个信号的值
    // 修改extractSignalValues方法，确保更新时间戳后触发自动滚动检查
    public void extractSignalValues() {
        // 重置时间戳范围
        earliestTimestamp = Double.MAX_VALUE;
        latestTimestamp = Double.MIN_VALUE;

        Map<String, Double> signalMinValues = new HashMap<>();
        Map<String, Double> signalMaxValues = new HashMap<>();

        // 首先按messageId对消息进行分组
        Map<String, List<CanMessage>> messagesByMessageId = new HashMap<>();
        for (CanMessage message : canMessages) {
            // 16进制的字符串
            String messageId = String.valueOf(message.getCanId());
            messagesByMessageId.computeIfAbsent(messageId, k -> new ArrayList<>()).add(message);

            // 解析时间戳并更新时间范围
            double timestamp = parseTimestamp(message.getTime());
            earliestTimestamp = Math.min(earliestTimestamp, timestamp);
            latestTimestamp = Math.max(latestTimestamp, timestamp);
        }

        for (DbcSignal signal : allSignals) {
            String messageId = signal.getMessageId();
            String id = messageId.replace("0X", "0x");
            // 转换为16进制字符串，不带0x前缀
            String canId = extractCanId(id);
            System.out.println("正在提取" + signal.getName() + "信号的值..." + "canId:" + canId);
            List<CanMessage> relevantMessages = messagesByMessageId.getOrDefault(canId, Collections.emptyList());
            List<Double> values = new ArrayList<>();
            List<Double> timestamps = new ArrayList<>(); // 存储该信号的时间戳

            for (CanMessage message : relevantMessages) {
                try {
                    double value = ExplanCanData.
                            extractSignalValue(ExplanCanData.hexStringToByteArray(message.getData()), signal);
                    values.add(value);

                    // 解析并存储时间戳
                    double timestamp = parseTimestamp(message.getTime());
                    timestamps.add(timestamp);
                } catch (Exception e) {
                    e.printStackTrace();
                    values.add(null); // 错误时添加null值
                    timestamps.add(null); // 保持索引一致
                }
            }

            signalValues.put(signal.getName(), values);
            System.out.println(signal.getName() + "提取到的值" + values.toString());
            signalTimestamps.put(signal.getName(), timestamps); // 保存时间戳
            Color color = signalColors.get(signal.getName());
            if (color == null) {
                color = Color.WHITE;
            }
            signalStrokeWidths.put(signal.getName(), 1f);
            signalValueVisibility.put(signal.getName(), showValueCheckBox.isSelected());

            String maximum = signal.getMaximum();
            String minimum = signal.getMinimum();
            //更改一下逻辑，采用values 中最大值 * 20% 作为最大值
            double max;
            if (!values.isEmpty()) {
                // 使用实际值中的最大值
                System.out.println("采用实际值中的最大值");
                max = values.stream().max(Double::compareTo).get() * 1.2;
            }else{
                // 使用信号定义的值
                System.out.println("采用信号定义的最大值");
                max = maximum == null ? Double.MAX_VALUE : Double.parseDouble(maximum);
            }


            double min = minimum == null ? Double.MIN_VALUE : Double.parseDouble(minimum);
            signalMaxValues.put(signal.getName(), max);
            signalMinValues.put(signal.getName(), min);


        }
        chartPanel.setSignalMinMaxValues(signalMinValues, signalMaxValues);
        // 计算最早和最晚时间戳
        calculateTimeRange();
        // 触发自动滚动检查（如果是首次加载数据）
        if (!signalValues.isEmpty() && offsetX == 0) {
            autoScrollToLatest();
        }
    }

    // 新增计算时间范围的方法（如果原有逻辑中没有的话）
    // 优化计算时间范围的方法，提升大数据量下的性能
    private void calculateTimeRange() {
        // 保留当前时间范围，用于后续比较
        double oldEarliest = earliestTimestamp;
        double oldLatest = latestTimestamp;

        // 初始化时间戳范围
        earliestTimestamp = Double.MAX_VALUE;
        latestTimestamp = Double.MIN_VALUE;
        boolean hasData = false;

        // 遍历所有信号的时间戳
        for (List<Double> timestamps : signalTimestamps.values()) {
            // 跳过空列表，提升性能
            if (timestamps.isEmpty()) {
                continue;
            }
            hasData = true;
            // 优化：只需要比较列表中的第一个和最后一个元素
            // 假设时间戳列表是按顺序添加的
            double first = timestamps.get(0);
            double last = timestamps.get(timestamps.size() - 1);
            earliestTimestamp = Math.min(earliestTimestamp, first);
            latestTimestamp = Math.max(latestTimestamp, last);
        }

        // 处理没有数据的情况
        if (!hasData) {
            earliestTimestamp = 0;
            latestTimestamp = 0;
        }

        // 只有当时间范围发生变化时才触发重绘检查
        if (earliestTimestamp != oldEarliest || latestTimestamp != oldLatest) {
            /// 首次加载数据时初始化偏移量，确保能看到完整数据起始部分
            if (oldEarliest == Double.MAX_VALUE) { // 判断是否是首次加载
                offsetX = 0; // 从最开始显示
                chartPanel.repaint();
            } else {
                autoScrollToLatest(); // 非首次加载时执行自动滚动
            }
        }
    }


    // 解析时间戳字符串为double类型
    private double parseTimestamp(String timestampStr) {
        try {
            return Double.parseDouble(timestampStr);
        } catch (NumberFormatException e) {
            e.printStackTrace();
            return 0.0; // 解析失败时返回0
        }
    }

    public String extractCanId(String fullCanId) {
        Pattern pattern = Pattern.compile("0x0*(.*)");
        Matcher matcher = pattern.matcher(fullCanId);
        if (matcher.matches()) {
            return matcher.group(1);
        }
        return fullCanId;
    }

    private void updateSignalVisibility() {
        List<String> selectedSignals = new ArrayList<>();
        for (Object selected : signalList.getSelectedValuesList()) {
            selectedSignals.add((String) selected);
        }
        for (String signalName : signalValues.keySet()) {
            signalVisibility.put(signalName, selectedSignals.contains(signalName));
        }
        chartPanel.repaint();
    }

    // 设置曲线是否显示值
    public void setSignalValueVisibility(String signalName, boolean visible) {
        signalValueVisibility.put(signalName, visible);
        chartPanel.repaint();
    }

    // 设置曲线背景颜色
    public void setSignalBackgroundColor(String signalName, Color color) {
        chartPanel.setSignalBackgroundColor(signalName, color);
        chartPanel.repaint();
    }

    // 设置曲线颜色
    public void setSignalColor(String signalName, Color color) {
        signalColors.put(signalName, color);
        chartPanel.repaint();
    }

    // 设置曲线粗细
    public void setSignalStrokeWidth(String signalName, float width) {
        signalStrokeWidths.put(signalName, width);
        chartPanel.repaint();
    }

    // 图表绘制面板
    private class ChartPanel extends JPanel {

        private static final int PADDING_TOP = 50;
        private static final int PADDING_BOTTOM = 50;
        private static final int TRIANGLE_SIZE = 5; // 小三角的大小
        private static final int GRID_SIZE = 50;
        private final Map<String, Color> COLORS = new HashMap<>();
        private final Map<String, Color> signalBackgroundColors = new ConcurrentHashMap<>();

        private Map<String, Double> signalMinValues;
        private Map<String, Double> signalMaxValues;

        public void setSignalMinMaxValues(Map<String, Double> minValues, Map<String, Double> maxValues) {
            this.signalMinValues = minValues;
            this.signalMaxValues = maxValues;
        }

        public ChartPanel() {
            // 预设一些颜色用于不同的信号曲线
            COLORS.put("S1", Color.RED);
            COLORS.put("S2", Color.BLUE);
            COLORS.put("S3", Color.GREEN);
            COLORS.put("S4", Color.ORANGE);
            COLORS.put("S5", Color.MAGENTA);
            COLORS.put("S6", Color.CYAN);
            COLORS.put("S7", Color.PINK);
            COLORS.put("S8", Color.YELLOW);
        }

        public void setSignalBackgroundColor(String signalName, Color color) {
            signalBackgroundColors.put(signalName, color);
        }

        @Override
        protected void paintComponent(Graphics g) {
            super.paintComponent(g);
            Graphics2D g2d = (Graphics2D) g;
            g2d.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);

            int width = getWidth();
            int height = getHeight();

            // 获取可见信号列表
            List<String> visibleSignals = new ArrayList<>();
            for (String signalName : signalValues.keySet()) {
                if (signalVisibility.getOrDefault(signalName, false)) {
                    visibleSignals.add(signalName);
                }
            }

            if (visibleSignals.isEmpty()) {
                // 没有可见信号，显示提示信息
                g2d.setFont(new Font("Arial", Font.PLAIN, 16));
                g2d.drawString("没有选择要显示的信号", width / 2 - 100, height / 2);
                return;
            }

            // 计算每个信号区域的高度
            int availableHeight = height - PADDING_TOP - PADDING_BOTTOM -
                    (visibleSignals.size() - 1) * SIGNAL_SPACING;
            int signalHeight = availableHeight / visibleSignals.size();

            // 计算当前可见的时间范围
            double visibleStartTime = earliestTimestamp + offsetX;
            double visibleEndTime = earliestTimestamp + offsetX + (width - Y_AXIS_WIDTH) / scaleX;

            // 绘制每个信号的图表
            for (int i = 0; i < visibleSignals.size(); i++) {
                String signalName = visibleSignals.get(i);
                int signalY = PADDING_TOP + i * (signalHeight + SIGNAL_SPACING);

                // 获取该信号的值和时间戳
                List<Double> values = signalValues.get(signalName);
                List<Double> timestamps = signalTimestamps.get(signalName);

                // 计算该信号在可见时间范围内的Y轴范围
//                double minValue = Double.MAX_VALUE;
//                double maxValue = Double.MIN_VALUE;

                double minValue = signalMinValues.getOrDefault(signalName, 0.0);
                double maxValue = signalMaxValues.getOrDefault(signalName, 0.0);

                // 确定可见范围内的数据索引
                int startIdx = findStartIndex(timestamps, visibleStartTime);
                int endIdx = findEndIndex(timestamps, visibleEndTime);

                for (int j = startIdx; j <= endIdx; j++) {
                    if (j < timestamps.size() && j < values.size()) {
                        Double value = values.get(j);
                        if (value != null) {
                            minValue = Math.min(minValue, value);
                            maxValue = Math.max(maxValue, value);
                        }
                    }
                }

                // 如果可见范围内没有数据，使用全部数据范围
                if (minValue == Double.MAX_VALUE || maxValue == Double.MIN_VALUE) {
                    for (Double value : values) {
                        if (value != null) {
                            minValue = Math.min(minValue, value);
                            maxValue = Math.max(maxValue, value);
                        }
                    }
                }

                // 确保有一定的范围
                double range = Math.max(0.001, maxValue - minValue);
                // 稍微扩展范围，使曲线不会紧贴Y轴边界
                double padding = range * 0.1;
                minValue -= padding;
                maxValue += padding;

                // 绘制该信号的图表区域
                drawSignalArea(g2d, width, signalHeight, signalY, signalName, minValue, maxValue, values, timestamps);
            }

            // 绘制共享的X轴（时间轴），考虑缩放和偏移
            drawSharedXAxis(g2d, width, height, visibleStartTime, visibleEndTime);
        }

        // 查找可见时间范围的开始索引
        private int findStartIndex(List<Double> timestamps, double startTime) {
            if (timestamps == null || timestamps.isEmpty()) return 0;

            int low = 0;
            int high = timestamps.size() - 1;
            int result = 0;

            while (low <= high) {
                int mid = (low + high) / 2;
                if (timestamps.get(mid) >= startTime) {
                    result = mid;
                    high = mid - 1;
                } else {
                    low = mid + 1;
                }
            }

            return Math.max(0, result - 1); // 稍微往前取一点，确保包含开始点
        }

        // 查找可见时间范围的结束索引
        private int findEndIndex(List<Double> timestamps, double endTime) {
            if (timestamps == null || timestamps.isEmpty()) return 0;

            int low = 0;
            int high = timestamps.size() - 1;
            int result = high;

            while (low <= high) {
                int mid = (low + high) / 2;
                if (timestamps.get(mid) <= endTime) {
                    result = mid;
                    low = mid + 1;
                } else {
                    high = mid - 1;
                }
            }

            return Math.min(timestamps.size() - 1, result + 1); // 稍微往后取一点，确保包含结束点
        }

        private void drawSignalArea(Graphics2D g2d, int width, int height, int y,
                                    String signalName, double minValue, double maxValue,
                                    List<Double> values, List<Double> timestamps) {
            // 获取或生成信号颜色
            Color color = signalColors.get(signalName);
            float strokeWidth = signalStrokeWidths.get(signalName);
            boolean showValues = signalValueVisibility.get(signalName);
            Color backgroundColor = signalBackgroundColors.getOrDefault(signalName, Color.WHITE);

            // 绘制背景和边框
            g2d.setColor(backgroundColor);
            g2d.fillRect(Y_AXIS_WIDTH, y, width - Y_AXIS_WIDTH, height);
            g2d.setColor(Color.LIGHT_GRAY);
            g2d.drawRect(Y_AXIS_WIDTH, y, width - Y_AXIS_WIDTH, height);

            // 绘制Y轴
            g2d.setColor(Color.BLACK);
            g2d.setStroke(new BasicStroke(1));
            g2d.drawLine(Y_AXIS_WIDTH, y, Y_AXIS_WIDTH, y + height);

            // 绘制Y轴标签
            g2d.setFont(new Font("Arial", Font.PLAIN, 15));
            g2d.setColor(Color.BLUE);
            g2d.drawString(signalName, 60, y+height/10);
            g2d.setFont(new Font("Arial", Font.PLAIN, 10));

            // 绘制Y轴刻度和标签
            int numYTicks = 5;
            for (int i = 0; i <= numYTicks; i++) {
                double yPos = y + height - (double) (i * height) / numYTicks;
                double value = minValue + i * (maxValue - minValue) / numYTicks;

                g2d.setColor(Color.BLACK);
                g2d.drawLine(Y_AXIS_WIDTH, (int) yPos, Y_AXIS_WIDTH - 5, (int) yPos);
                g2d.drawString(String.format("%.2f", value), 5, (int) yPos + 4);
            }

            // 绘制网格线
            g2d.setColor(Color.LIGHT_GRAY);
            // 绘制曲线
            g2d.setColor(Color.black);
            g2d.setColor(color);
            g2d.setStroke(new BasicStroke(strokeWidth));

            double prevX = 0, prevY = 0;
            boolean hasPrev = false;

            // 计算当前可见的时间范围
            double visibleStartTime = earliestTimestamp + offsetX;
            double visibleEndTime = earliestTimestamp + offsetX + (width - Y_AXIS_WIDTH) / scaleX;

            // 确定可见范围内的数据索引
            int startIdx = findStartIndex(timestamps, visibleStartTime);
            int endIdx = findEndIndex(timestamps, visibleEndTime);

            for (int i = startIdx; i <= endIdx; i++) {
                if (i < timestamps.size() && i < values.size()) {
                    Double timestamp = timestamps.get(i);
                    Double value = values.get(i);

                    if (timestamp == null || value == null) {
                        hasPrev = false;
                        continue;
                    }

                    // 计算坐标，考虑缩放和偏移
                    double x = Y_AXIS_WIDTH + (timestamp - earliestTimestamp - offsetX) * scaleX;
                    double yPos = y + height - (value - minValue) * height / (maxValue - minValue);

                    // 检查点是否在可见区域内
                    if (x >= Y_AXIS_WIDTH && x <= width &&
                            yPos >= y && yPos <= y + height) {
                        if (hasPrev) {
                            g2d.drawLine((int) prevX, (int) prevY, (int) x, (int) yPos);
                        }
                        hasPrev = true;
                        prevX = x;
                        prevY = yPos;

                        // 如果需要显示值，绘制值
                        if (showValues) {
                            g2d.setFont(new Font("Arial", Font.PLAIN, 10));
                            g2d.setColor(color);
                            g2d.drawString(String.format("%.2f", value), (int) x + 2, (int) yPos - 2);
                        }
                    }
                }
            }
        }

        private void drawSharedXAxis(Graphics2D g2d, int width, int height, double startTime, double endTime) {
            // 绘制共享的X轴（时间轴）
            g2d.setColor(Color.BLACK);
            g2d.setStroke(new BasicStroke(2));
            g2d.drawLine(Y_AXIS_WIDTH, height - PADDING_BOTTOM, width, height - PADDING_BOTTOM);

            // 绘制X轴标签
            g2d.setFont(new Font("Arial", Font.PLAIN, 12));
            g2d.drawString("时间", width / 2, height - PADDING_BOTTOM + 20);

            // 根据时间范围动态调整刻度数量和格式
            double visibleRange = endTime - startTime;
            int numXTicks = calculateNumXTicks(visibleRange);

            for (int i = 0; i <= numXTicks; i++) {
                double timeValue = startTime + i * visibleRange / numXTicks;
                double x = Y_AXIS_WIDTH + (timeValue - earliestTimestamp - offsetX) * scaleX;

                g2d.drawLine((int) x, height - PADDING_BOTTOM, (int) x, height - PADDING_BOTTOM + 5);

//                // 格式化时间标签
//                String timeLabel = formatTime(timeValue, timeUnit);

                // 格式化时间标签
                String timeLabel = TimeFormater.formatTimeToHMSMS(timeValue);
                System.out.println(timeValue);
                System.out.println("timeLabel: " + timeLabel);
                g2d.drawString(timeLabel, (int) x - 15, height - PADDING_BOTTOM + 20);
            }
        }

        // 计算合适的刻度数量
        private int calculateNumXTicks(double visibleRange) {
            if (visibleRange <= 1) {
                return 5; // 非常小的范围，显示5个刻度
            } else if (visibleRange <= 10) {
                return 10; // 小范围，显示10个刻度
            } else if (visibleRange <= 60) {
                return 6; // 中等范围，显示6个刻度
            } else {
                return Math.min(10, (int) Math.ceil(visibleRange / 10)); // 大范围，最多10个刻度
            }
        }

        // 确定时间单位
        private TimeUnit determineTimeUnit(double range) {
            if (range < 1) {
                return TimeUnit.MILLISECONDS;
            } else if (range < 60) {
                return TimeUnit.SECONDS;
            } else if (range < 3600) {
                return TimeUnit.MINUTES;
            } else {
                return TimeUnit.HOURS;
            }
        }

        // 获取时间格式化器
        private DecimalFormat getTimeFormat(TimeUnit unit) {
            switch (unit) {
                case MILLISECONDS:
                    return new DecimalFormat("0.000");
                case SECONDS:
                    return new DecimalFormat("0.0");
                case MINUTES:
                    return new DecimalFormat("0.0");
                case HOURS:
                    return new DecimalFormat("0.0");
                default:
                    return new DecimalFormat("0");
            }
        }

        // 格式化时间值
        private String formatTime(double timeValue, TimeUnit unit) {
            double convertedValue = timeValue;
            String unitLabel = "";

            switch (unit) {
                case MILLISECONDS:
                    convertedValue *= 1000; // 转换为毫秒
                    unitLabel = "ms";
                    break;
                case SECONDS:
                    unitLabel = "s";
                    break;
                case MINUTES:
                    convertedValue /= 60; // 转换为分钟
                    unitLabel = "min";
                    break;
                case HOURS:
                    convertedValue /= 3600; // 转换为小时
                    unitLabel = "h";
                    break;
            }

            DecimalFormat format = getTimeFormat(unit);
            return format.format(convertedValue) + unitLabel;
        }

    }

    // 创建模拟的DbcMessage列表
    private static List<DbcMessage> createSampleDbcMessages() {
        List<DbcMessage> dbcMessages = new ArrayList<>();

        // 创建几个示例消息定义
        DbcMessage msg1 = new DbcMessage();
        msg1.setMessageId("0");
        msg1.setCycleTime("10"); // 10ms周期
        dbcMessages.add(msg1);

        DbcMessage msg2 = new DbcMessage();
        msg2.setMessageId("1");
        msg2.setCycleTime("20"); // 20ms周期
        dbcMessages.add(msg2);

        DbcMessage msg3 = new DbcMessage();
        msg3.setMessageId("2");
        msg3.setCycleTime("50"); // 50ms周期
        dbcMessages.add(msg3);

        return dbcMessages;
    }

    public static void main(String[] args) {
        SwingUtilities.invokeLater(new Runnable() {
            @Override
            public void run() {
                // 创建示例DbcMessage数据
                List<DbcMessage> dbcMessages = createSampleDbcMessages();
                // 创建示例数据
                List<CanMessage> canMessages = createSampleMessages(1000, dbcMessages);
                List<DbcSignal> dbcSignals = createSampleSignals();
                CanMessageGenerator generator = new CanMessageGenerator("2A6");
                SignalChartDrawer drawer =
                        new SignalChartDrawer(generator.generateMessages(100), new ArrayList<>());
                drawer.setVisible(true);
            }
        });


    }

    private static List<CanMessage> createSampleMessages(int count, List<DbcMessage> dbcMessages) {
        List<CanMessage> messages = new ArrayList<>();

        // 为每种消息类型维护一个时间戳计数器
        Map<String, Integer> timestamps = new HashMap<>();

        // 生成模拟消息
        for (int i = 0; i < count; i++) {
            // 为每种消息类型生成消息（根据其周期）
            for (DbcMessage dbcMessage : dbcMessages) {
                String messageId = dbcMessage.getMessageId();

                // 获取消息周期（默认为100ms）
                int period = 100;
                try {
                    period = Integer.parseInt(dbcMessage.getCycleTime());
                } catch (NumberFormatException e) {
                    // 使用默认周期
                }

                // 根据周期决定是否在此时生成此消息
                if (i % period == 0) {
                    CanMessage message = new CanMessage();
                    message.setId(Long.parseLong(messageId));

                    // 更新该消息的时间戳
                    int timestamp = timestamps.getOrDefault(messageId, 0) + period;
                    timestamps.put(messageId, timestamp);

                    // 设置时间戳字符串（格式化为带小数的字符串）
                    message.setTime(String.format("%.6f", timestamp / 1000.0));

                    // 生成更真实的信号数据
                    byte[] data = new byte[8];

                    // 基于时间生成模拟的物理值（使用正弦波模拟变化）
                    double baseValue = 50.0; // 默认基准值
                    double amplitude = 30.0; // 默认振幅
                    double frequency = 0.01; // 变化频率

                    // 填充数据（假设前4个字节为主要信号值）
                    double value = baseValue + amplitude * Math.sin(i * frequency);
                    long intValue = (long) (value * 100); // 转换为整数表示

                    for (int j = 0; j < 4; j++) {
                        data[j] = (byte) ((intValue >> (j * 8)) & 0xFF);
                    }

                    // 添加一些随机噪声，使数据更真实
                    for (int j = 4; j < 8; j++) {
                        data[j] = (byte) (Math.random() * 256);
                    }

                    message.setData(new String(data));
                    messages.add(message);
                }
            }
        }

        // 对消息按时间排序（确保消息按发送顺序排列）
        messages.sort(Comparator.comparingDouble(m -> Double.parseDouble(m.getTime())));

        return messages;
    }

    private static List<DbcSignal> createSampleSignals() {
        List<DbcSignal> signals = new ArrayList<>();

        // 创建模拟信号定义
        for (int i = 0; i < 8; i++) {
            DbcSignal signal = new DbcSignal();
            signal.setName("Signal" + (i + 1));
            signal.setMessageId(String.valueOf(i % 3)); // 3种不同的消息ID
            signal.setStartBit(String.valueOf(i * 2));
            signal.setLength(String.valueOf(8));
            signal.setScale(String.valueOf(1.0));
            signal.setOffset(String.valueOf(0.0));
            signals.add(signal);
        }

        return signals;
    }

}