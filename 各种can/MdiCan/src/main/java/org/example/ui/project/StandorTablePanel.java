/*
 * Created by JFormDesigner on Mon Jun 30 14:00:00 CST 2025
 */

package org.example.ui.project;

import java.awt.event.*;

import cn.hutool.core.bean.BeanUtil;
import com.alibaba.excel.EasyExcel;
import com.alibaba.excel.write.builder.ExcelWriterBuilder;
import com.baomidou.mybatisplus.core.conditions.query.QueryWrapper;
import com.baomidou.mybatisplus.extension.plugins.pagination.Page;
import org.apache.ibatis.session.ExecutorType;
import org.apache.poi.ss.formula.functions.Even;
import org.apache.poi.ss.formula.functions.T;
import org.example.mapper.CProjectMapper;
import org.example.mapper.CanMessageMapper;
import org.example.pojo.data.CanMessage;
import org.example.pojo.data.SendMessage;
import org.example.pojo.project.CProject;
import org.example.ui.MainFrame;
import org.example.ui.project.chart.SignalChartDrawer;
import org.example.ui.project.chart.bitView.CanFdBitViewer;
import org.example.ui.project.chart.freeChart.ChartContainerForm;
import org.example.ui.project.chart.freeChart.DraggableTimeChart;
import org.example.ui.project.error.ErrorInfoDisplay;
import org.example.ui.project.filter.ReciverFilterConfigView;
import org.example.ui.project.search.CanIdSearchForm;
import org.example.ui.project.send.SendMessageDialog;
import org.example.ui.project.waitting.WaittingView;
import org.example.utils.db.BatchSqlProvider;
import org.example.utils.db.BlfParser;
import org.example.utils.db.DatabaseManager;
import org.example.utils.db.TableNameContextHolder;
import org.example.utils.event.EventListener;
import org.example.utils.event.EventManager;
import org.example.utils.event.EventString;
import org.example.utils.file.LargeFileReader;
import org.example.utils.proto.CanProtobufReader;
import org.example.utils.table.EnhancedTable;
import org.example.utils.web.ReceivedDataReCorder;
import org.example.utils.web.SaveMessageToDb;
import org.example.utils.web.UdpWifiDataReceiver;
import org.example.utils.web.UdpWifiReceiverManager;
import org.example.utils.web.UsbSerialDataReceiver;
import org.example.utils.web.UsbSerialReceiverManager;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.awt.*;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.time.Instant;
import java.time.format.DateTimeFormatter;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Objects;
import java.util.concurrent.*;
import java.util.concurrent.atomic.AtomicBoolean;
import javax.swing.*;
import javax.swing.border.*;
import javax.swing.filechooser.FileNameExtensionFilter;

/**
 * @author jingq
 */
public class StandorTablePanel extends JPanel{
    private static final Logger logger = LoggerFactory.getLogger(StandorTablePanel.class);

    private boolean isModify;

    private boolean isSaved;

    private SendMessageDialog sendMultipleConfigDialog;

    private CanIdSearchForm canIdSearchForm;

    private Timer timer;
    public StandorTablePanel(Long projectId) {
        setSize(getMaximumSize());
        this.projectId = projectId;
        initComponents();
        initUserComponents();
        logger.info("StandorTablePanel init 初始化");
        // 加载历史数据
        loadDataByProjectId();
        sendMultipleConfigDialog = new SendMessageDialog(null);
        sendMultipleConfigDialog.setProjectId(projectId);
        setTableRightClickMenu(enhancedTable);
        //批量设置按钮的大小和效果
        processAllButtons(this);
        registerSearchShortcut();
        // 定时任务，每一秒中更新一下速度信息
        timer = new Timer(1000, new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                // 更新速度信息
                double speed = ReceivedDataReCorder.getInstance().getEveSpeed(projectId);
                eveSpeedLabel.setText(String.format("%.2f M bit/s", speed));
                maxSpeedLabel.setText(String.format("%.2f M bit/s", ReceivedDataReCorder.getInstance().getMaxSpeed(projectId)));
                ReceivedDataReCorder.getInstance().clearProjectReceivedSpeedMap(projectId);
            }
        });
        timer.start();
    }

    private void registerSearchShortcut() {
        // 获取面板的输入映射（当面板拥有焦点时生效）
        InputMap inputMap = getInputMap(JComponent.WHEN_IN_FOCUSED_WINDOW);
        // 获取面板的动作映射
        ActionMap actionMap = getActionMap();

        // 定义Ctrl+F快捷键 (KeyStroke表示按键组合)
        KeyStroke ctrlF = KeyStroke.getKeyStroke("control F");

        // 定义快捷键对应的动作
        Action searchAction = new AbstractAction() {
            @Override
            public void actionPerformed(ActionEvent e) {
                // 这里实现搜索功能，例如弹出搜索对话框
                canIdSearchForm = new CanIdSearchForm(null, enhancedTable.getTableData(), new CanIdSearchForm.ReturnValueListener() {
                    @Override
                    public void onReturnValue(String value) {
                        // 这里处理搜索结果
                        enhancedTable.scrollToAndSelectRow(Integer.parseInt(value));
                    }
                });
                canIdSearchForm.setVisible(true);
            }
        };

        // 将快捷键与动作关联
        inputMap.put(ctrlF, "searchAction");
        actionMap.put("searchAction", searchAction);
    }


    public UdpWifiDataReceiver getUppWifiDataReceiver(){
        return uppWifiDataReceiver;
    }
    
    public EnhancedTable<CanMessage> getEnhancedTable() {
        return enhancedTable;
    }
    private void initUserComponents(){
        enhancedTable = new EnhancedTable<>(CanMessage.class);
        JPanel centerPanel = new JPanel();
        centerPanel.setLayout(new BorderLayout());
        centerPanel.add(enhancedTable.getFilterPanel(), BorderLayout.NORTH);
        JScrollPane scrollPane = new JScrollPane(enhancedTable);
        centerPanel.add(scrollPane, BorderLayout.CENTER);
        add(centerPanel, BorderLayout.CENTER);
        // 配置过滤器
        reciverFilter = new ReciverFilterConfigView(projectId);
        targetIds = reciverFilter.getCurrentMessageIds();
        // 配置按钮事件
        filterBtn.addActionListener(e -> {
            reciverFilter.setVisible(true);
        });
        // 注册过滤器更改事件监听器
        EventManager.getInstance().addListener(new EventListener() {
            @Override
            public void onEvent(String event) {
                if (Objects.equals(event, EventString.UPDATE_TABLE_FILTER_CONFIG.toString())) {
                    targetIds = reciverFilter.getCurrentMessageIds();
                    logger.info("更新过滤条件");
                }
            }
        });
        EventManager.getInstance().addListener(new EventListener() {
            @Override
            public void onEvent(String event) {
                if (event.startsWith(EventString.REFRESS_RECEIVE_DATA_COUNTER.name())) {
                    String[] split = event.split("#");
                    String id = split[1];
                    String counter = split[2];
                    if(projectId.equals(Long.parseLong(id)) && !String.valueOf(counter).equals(dataCounter.getText())){
                        SwingUtilities.invokeLater(()->{
                            dataCounter.setText(counter);
                        });
                    }
                }
            }
        });
    }

    private void setTableRightClickMenu(EnhancedTable<CanMessage> table) {
        // 添加右键点击事件监听器
        table.addMouseListener(new MouseAdapter() {
            @Override
            public void mousePressed(MouseEvent e) {
                maybeShowPopup(e);
            }

            @Override
            public void mouseReleased(MouseEvent e) {
                maybeShowPopup(e);
            }

            private void maybeShowPopup(MouseEvent e) {
                if (e.isPopupTrigger()) {
                    int row = table.rowAtPoint(e.getPoint());
                    int col = table.columnAtPoint(e.getPoint());
                    if (row != -1 && col != -1) {
                        JPopupMenu popupMenu = getjPopupMenu(row);
                        popupMenu.show(e.getComponent(), e.getX(), e.getY());
                    }
                }
            }
        });
    }

    private JPopupMenu getjPopupMenu(int row) {
        JPopupMenu popupMenu = new JPopupMenu();
        JMenuItem menuItem = new JMenuItem("追踪消息值变化");
        menuItem.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                CanMessage rowData = enhancedTable.getDataAt(row);
                // 在这里添加菜单项的逻辑
                System.out.println("追踪消息值变化");
                JOptionPane.showMessageDialog(null, "此功能尚未开发...");
            }
        });
        JMenuItem addToSendListMenuItem = new JMenuItem("加入发送清单");
        addToSendListMenuItem.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                CanMessage rowData = enhancedTable.getDataAt(row);
                // 在这里添加菜单项的逻辑
                System.out.println("加入发送清单");
                if(sendMultipleConfigDialog ==  null){
                    sendMultipleConfigDialog = new SendMessageDialog(null);
                }
                sendMultipleConfigDialog.setDataReceiver(uppWifiDataReceiver);
                for (int selectedRow : enhancedTable.getSelectedRows()) {
                    //CanMessage canMessage = enhancedTable.getDataAt(selectedRow);
                    CanMessage canMessage = enhancedTable.getFilteredData().get(selectedRow);
                    sendMultipleConfigDialog.append(canMessage);
                }
                sendMultipleConfigDialog.setVisible(true);
            }
        });
        JMenuItem addAsExceptMenuItem = new JMenuItem("加入排除清单");
        addAsExceptMenuItem.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                CanMessage rowData = enhancedTable.getDataAt(row);
                // 在这里添加菜单项的逻辑
                System.out.println("加入排除清单");
            }
        });
        JMenuItem addAllMenuItem = new JMenuItem("加入全量数据");
        addAllMenuItem.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                System.out.println("加入全量数据");
                if(sendMultipleConfigDialog ==  null){
                    sendMultipleConfigDialog = new SendMessageDialog(null);
                }
                sendMultipleConfigDialog.setDataReceiver(uppWifiDataReceiver);
                // 获取表格的全量数据
                sendMultipleConfigDialog.appendBatch(enhancedTable.getTableData());
                sendMultipleConfigDialog.setVisible(true);
            }
        });
        JMenuItem exportMenuItem = new JMenuItem("导出全量(Excel)");
        exportMenuItem.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                // 在这里添加菜单项的逻辑
                System.out.println("导出全量");
                // 打开文件保存文件夹对话框
                JFileChooser fileChooser = new JFileChooser();
                fileChooser.setFileSelectionMode(JFileChooser.FILES_AND_DIRECTORIES);
                //设置默认的文件名
                fileChooser.setCurrentDirectory(new File("./"));
                int returnValue = fileChooser.showSaveDialog(null);
                if (returnValue == JFileChooser.APPROVE_OPTION) {
                    File selectedFile = fileChooser.getSelectedFile();
                    System.out.println("Selected directory: " + selectedFile.getAbsolutePath());
                    // 导出全量数据
                    EasyExcel.write(selectedFile.getAbsolutePath() , CanMessage.class)
                            .sheet("全量数据")
                            .doWrite(loadAll());
                    JOptionPane.showMessageDialog(null, "导出成功！" + selectedFile.getAbsoluteFile(),
                            "提示", JOptionPane.INFORMATION_MESSAGE);
                }

            }
        });
        JMenuItem exportToTextMenuItem = new JMenuItem("导出TEXT");
        exportToTextMenuItem.addActionListener(new ActionListener() {

            @Override
            public void actionPerformed(ActionEvent e) {
                JFileChooser fileChooser = new JFileChooser();
                fileChooser.setFileSelectionMode(JFileChooser.FILES_AND_DIRECTORIES);
                //设置默认的文件名
                fileChooser.setCurrentDirectory(new File("./"));
                int returnValue = fileChooser.showSaveDialog(null);
                if (returnValue == JFileChooser.APPROVE_OPTION) {
                    File selectedFile = fileChooser.getSelectedFile();
                    System.out.println("Selected directory: " + selectedFile.getAbsolutePath());
                    // 导出全量数据
                    exportDataToText(selectedFile.getAbsolutePath(), loadAll());
                    JOptionPane.showMessageDialog(null, "导出成功！" + selectedFile.getAbsoluteFile(),
                            "提示", JOptionPane.INFORMATION_MESSAGE);
                }
            }
        });
        JMenuItem bitViewMenuItem = new JMenuItem("显示位图");
        bitViewMenuItem.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                System.out.println("显示位图");
                int selectedRow = enhancedTable.getSelectedRow();
                CanMessage canMessage = enhancedTable.getFilteredData().get(selectedRow);
                SendMessage sendMessage = new SendMessage();
                BeanUtil.copyProperties(canMessage, sendMessage);
                // 在这里添加菜单项的逻辑
                CanFdBitViewer.showCanData(sendMessage);
            }
        });

        popupMenu.add(menuItem);
        popupMenu.addSeparator();
        popupMenu.add(addToSendListMenuItem);
        popupMenu.add(addAsExceptMenuItem);
        popupMenu.add(addAllMenuItem);
        popupMenu.addSeparator();
        popupMenu.add(exportMenuItem);
        popupMenu.add(exportToTextMenuItem);
        popupMenu.add(bitViewMenuItem);
        return popupMenu;
    }
    // 导出数据到TEXT
    private void exportDataToText(String absolutePath, List<CanMessage> tableData) {
        try (BufferedWriter writer = new BufferedWriter(new FileWriter(absolutePath))) {
            for (CanMessage message : tableData) {
                writer.write(message.getCanId()
                        + "," + message.getData()
                        + "," + message.getTime()
                        + "," + message.getIndexCounter()
                        + "\n");
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private void startBtnHandler(ActionEvent e) {
        // TODO add your code here
        startReceiving();
    }

    public void setProjectId(Long projectId) {
        this.projectId = projectId;
    }

    private void loadDataByProjectId() {
        // 检查表是否存在
        SaveMessageToDb saveMessageToDb = new SaveMessageToDb();
        saveMessageToDb.checkAndCreateTable(projectId);
        // 获取数据库连接
        DatabaseManager databaseManager = new DatabaseManager();
        List<CanMessage> messages = databaseManager.execute(CanMessage.class, session -> {
            CanMessageMapper canMessageMapper = session.getMapper(CanMessageMapper.class);
            QueryWrapper<CanMessage> queryWrapper = new QueryWrapper<>();
            try{
                TableNameContextHolder.setSuffix("can_message" , "_p_" + projectId);
                // 获取总页数
                Long total = canMessageMapper.selectCount(queryWrapper);
                // 按照分页参数查询
                Object selectedItem = pageSizeComb.getSelectedItem();
                int pageSize = selectedItem == null ? 100 : Integer.parseInt(selectedItem.toString());
                Object selectedItem1 = pagesComb.getSelectedItem();
                int currentPage = selectedItem1==null ? 1 : Integer.parseInt(selectedItem1.toString());
                Page<CanMessage> page = new Page<>(currentPage, pageSize);
                Page<CanMessage> canMessagePage = canMessageMapper.selectPage(page, queryWrapper);
                // 设置相关标签值
                totalLable.setText(total.toString());
                long totalPage = total/pageSize;
                pagesComb.removeAllItems();
                for (int i = 1; i <= totalPage; i++) {
                   pagesComb.addItem(String.valueOf(i));
                }
                return canMessagePage.getRecords();
            }finally {
                TableNameContextHolder.clearSuffix("can_message");
            }

        });
        // 批量更新表格
        enhancedTable.removeAll();
        enhancedTable.appendBatch(messages);
    }

    private List<CanMessage> loadAll() {
        // 获取数据库连接
        DatabaseManager databaseManager = new DatabaseManager();
        List<CanMessage> messages = databaseManager.execute(CanMessage.class, session -> {
            CanMessageMapper canMessageMapper = session.getMapper(CanMessageMapper.class);
            QueryWrapper<CanMessage> queryWrapper = new QueryWrapper<>();
            try{
                TableNameContextHolder.setSuffix("can_message" , "_p_" + projectId);

                return canMessageMapper.selectList(queryWrapper);
            }finally {
                TableNameContextHolder.clearSuffix("can_message");
            }

        });
        return messages;
    }

    public void setParentFrame(MainFrame parentFrame) {
        this.parentFrame = parentFrame;
    }

    public long getProjectId() {
        return projectId;
    }
    
    /**
     * 获取所有可用的通信通道（UDP端口 + USB串口）
     */
    private String[] getAvailableChannels() {
        java.util.List<String> channels = new java.util.ArrayList<>();
        
        // 第一项：刷新选项
        channels.add("🔄 刷新端口列表...");
        
        // 添加UDP端口
        channels.add("UDP: port 8077");
        channels.add("UDP: port 8078");
        channels.add("UDP: port 8079");
        channels.add("UDP: port 8081");
        channels.add("UDP: port 8082");
        
        // 添加USB串口
        try {
            String[] usbPorts = UsbSerialDataReceiver.getAvailablePorts();
            for (String port : usbPorts) {
                channels.add("USB: " + port);
            }
        } catch (Exception e) {
            logger.warn("获取USB串口列表失败: {}", e.getMessage());
        }
        
        return channels.toArray(new String[0]);
    }
    
    /**
     * 刷新通道列表
     */
    public void refreshChannelList() {
        String[] channels = getAvailableChannels();
        channelBox.setModel(new DefaultComboBoxModel<>(channels));
        // 默认选择第二项（跳过刷新选项）
        if (channels.length > 1) {
            channelBox.setSelectedIndex(1);
        }
        logger.info("通道列表已刷新，共{}个通道", channels.length - 1);
    }
    
    /**
     * 通道选择变化处理
     */
    private void channelBoxSelectionChanged() {
        Object selected = channelBox.getSelectedItem();
        if (selected != null && selected.toString().startsWith("🔄")) {
            // 选择了刷新选项，执行刷新
            refreshChannelList();
        }
    }

    private void startReceiving(){
        if(isReceiving) return;
        Object selectedItem = channelBox.getSelectedItem();
        if(selectedItem == null){
            JOptionPane.showMessageDialog(this, "请选择一个端口", "错误", JOptionPane.ERROR_MESSAGE);
            return;
        }
        String selection = selectedItem.toString();
        
        // 如果选择的是刷新选项，不启动
        if (selection.startsWith("🔄")) {
            refreshChannelList();
            return;
        }
        
        // 判断是USB还是UDP模式
        if (selection.startsWith("USB:")) {
            // USB串口模式
            startUsbReceiving(selection);
        } else {
            // UDP模式（保持原有逻辑）
            startUdpReceiving(selection);
        }
    }
    
    /**
     * 启动USB串口接收
     */
    private void startUsbReceiving(String selection) {
        // 提取端口名 (格式: "USB: COM3 - USB Serial Device")
        String portInfo = selection.substring(5).trim();
        String portName = portInfo.split(" - ")[0].trim();
        
        logger.info("启动USB串口接收: {}", portName);
        startBtn.setEnabled(false);
        stopBtn.setEnabled(true);
        isReceiving = true;
        communicationMode = "USB";
        
        // 创建USB接收器
        usbSerialDataReceiver = new UsbSerialDataReceiver(portName);
        usbSerialDataReceiver.setMainFrame(this);
        
        // 加入管理器
        if (!UsbSerialReceiverManager.getInstance().hasReceiver(portName)) {
            UsbSerialReceiverManager.getInstance().addReceiver(portName, usbSerialDataReceiver);
        }
        
        // 启动定时更新UI
        if (scheduler.isShutdown() || scheduler == null) {
            scheduler = Executors.newSingleThreadScheduledExecutor();
            logger.info("启动定时更新UI");
        }
        scheduler.scheduleAtFixedRate(this::processReceivedDataBatch, 0,
                UI_UPDATE_INTERVAL_MS, TimeUnit.MILLISECONDS);
        
        // 启动USB接收
        if (usbSerialDataReceiver.start()) {
            logger.info("USB串口接收已启动: {}", portName);
            parentFrame.setStatusBarText("正在通过USB接收CAN数据: " + portName);
        } else {
            logger.error("USB串口启动失败: {}", portName);
            JOptionPane.showMessageDialog(this, "USB串口启动失败: " + portName, 
                "错误", JOptionPane.ERROR_MESSAGE);
            stopBtnHandler(null);
        }
    }
    
    /**
     * 启动UDP接收（原有逻辑）
     */
    private void startUdpReceiving(String selection) {
        // 解析端口号 (格式: "UDP: port 8077")
        String port = selection.replaceAll("[^0-9]", "");
        logger.info("start receiving from wifi,port: " + port);
        // 启动按钮置灰
        startBtn.setEnabled(false);
        stopBtn.setEnabled(true);
        isReceiving = true;
        communicationMode = "UDP";
        
        // 启动数据接收器
        uppWifiDataReceiver = new UdpWifiDataReceiver(Integer.parseInt(port));
        // 加入实例管理器
        if(!UdpWifiReceiverManager.getInstance().hasReceiver(Integer.parseInt(port))){
            UdpWifiReceiverManager.getInstance().addReceiver(Integer.parseInt(port),uppWifiDataReceiver);
        }
        uppWifiDataReceiver.setMainFrame(this);
        // 启动定时更新UI
        if(scheduler.isShutdown() || scheduler == null){
            scheduler = Executors.newSingleThreadScheduledExecutor();
            logger.info("启动定时更新UI");
        }
        scheduler.scheduleAtFixedRate(this::processReceivedDataBatch,0,
                UI_UPDATE_INTERVAL_MS,TimeUnit.MILLISECONDS);

        uppWifiDataReceiver.start();
        logger.info("正在接收CAN数据...,port " + port );
        parentFrame.setStatusBarText("正在接收CAN数据...,port " + port );
    }

    public void updateTable(){
        ArrayList<CanMessage> messagesToProcess = new ArrayList<>(messageBuffer);
        messageBuffer.clear();
        // 批量更新表格
        enhancedTable.appendBatch(messagesToProcess);
    }

    // 批量处理接收到的数据
    private void processReceivedDataBatch() {
        try {
//            logger.info("启动处理数据");
            if(dataQueue.isEmpty()){
//                logger.info("队列为空");
                return;
            }
            // 批量处理队列中的数据
            List<byte[]> batchData = new ArrayList<>();
            dataQueue.drainTo(batchData, MAX_PROCESS_ONE_TIME);
            // 如果队列中仍有大量数据，暂停UI更新以加快处理速度
            if (dataQueue.size() > MAX_QUEUE_SIZE / 2) {
                uiUpdateEnabled.set(false);
                logger.info("数据量过大，暂停UI更新");
            }
            for (byte[] data : batchData) {
                List<CanMessage> messages = CanMessage.parse(data);
                for (CanMessage message : messages) {
                    // 加入过滤
                    if (!targetIds.isEmpty() && !targetIds.contains(message.getCanId())) {
                        continue;
                    }
                    message.setProjectId(projectId);
                    messageBuffer.add(message);
                }
            }
            logger.info("缓存中数据量：" + messageBuffer.size());
            logger.info("处理后队列中数据量：" + dataQueue.size());
            // 恢复UI更新
            if (!uiUpdateEnabled.get() && dataQueue.size() < MAX_QUEUE_SIZE / 4) {
                logger.info("恢复UI更新");
                uiUpdateEnabled.set(true);
            }
            // 批量更新UI
            if (!messageBuffer.isEmpty() && uiUpdateEnabled.get()) {
                logger.info("更新数据到表格");
                SwingUtilities.invokeLater(this::updateTable);
            }
        }catch (Exception e){
            e.printStackTrace();
            logger.error("数据处理异常", e);
        }



    }


    private void stopBtnHandler(ActionEvent e) {
        // TODO add your code here
        if(!isReceiving) return;
        isReceiving = false;
        startBtn.setEnabled(true);
        stopBtn.setEnabled(false);
        scheduler.shutdown();

        try {
            if (!scheduler.awaitTermination(1, TimeUnit.SECONDS)) {
                scheduler.shutdownNow();
            }
        } catch (InterruptedException exception) {
            scheduler.shutdownNow();
        }
        
        // 根据通信模式停止对应的接收器
        if ("USB".equals(communicationMode) && usbSerialDataReceiver != null) {
            usbSerialDataReceiver.stop();
            usbSerialDataReceiver = null;
            logger.info("USB串口接收已停止");
        } else if (uppWifiDataReceiver != null) {
            uppWifiDataReceiver.stop();
            logger.info("UDP接收已停止");
        }
        
        dataQueue.clear();
        messageBuffer.clear();
        parentFrame.setStatusBarText("已停止接收");
    }

    private void saveHandler(ActionEvent e) {

        // TODO add your code here
        WaittingView  waittingView = new WaittingView(null);
        waittingView.show();
        List<CanMessage> data = enhancedTable.getTableData();
        List<CanMessage> toSave = new ArrayList<>();
        List<CanMessage> toUpdate = new ArrayList<>();
        for (CanMessage message : data) {
            if (message.getId() == null) {
                toSave.add(message);
            } else {
                toUpdate.add(message);
            }
        }
        DatabaseManager databaseManager = new DatabaseManager();
        // 新增
        if(!toSave.isEmpty()){
            databaseManager.execute(CanMessage.class,session -> {
                int total = 0 ;
                CanMessageMapper mapper = session.getMapper(CanMessageMapper.class);
                List<List<CanMessage>> lists = BatchSqlProvider.splitBatch(toSave,500);
                for(List<CanMessage> list : lists){
                    total += mapper.insertBatch(list);
                }
                session.commit();
                return total;
            },false, ExecutorType.BATCH);
        }
        // 更新
        List<CanMessage> changedList = enhancedTable.getChangedList();
        if(!changedList.isEmpty()){
            databaseManager.execute(CanMessage.class,session -> {
                CanMessageMapper mapper = session.getMapper(CanMessageMapper.class);
                for (CanMessage message : changedList) {
                    mapper.update(message,new QueryWrapper<CanMessage>().eq("id",message.getId()));
                }
                return true;
            });
        }
        waittingView.close();
        JOptionPane.showMessageDialog(this, "保存成功", "提示", JOptionPane.INFORMATION_MESSAGE);
    }

    private void trackSignalsHandler(ActionEvent e) {
        
        // new Thread(this::startChartView).start();
        new Thread(this::startChartView2).start();
    }

    private void startChartView(){
        SwingUtilities.invokeLater(() -> {
            SignalChartDrawer signalChartDrawer =
                    new SignalChartDrawer(enhancedTable.getTableData(), new ArrayList<>());
            signalChartDrawer.setVisible(true);
            // 设置定时任务
            ScheduledExecutorService executor = Executors.newSingleThreadScheduledExecutor();
            executor.scheduleAtFixedRate(() -> {
                List<CanMessage> tableData = enhancedTable.getTableData();
                SwingUtilities.invokeLater(() -> {
                    if(signalChartDrawer.isVisible()){
                        // 更新数据和曲线
                        signalChartDrawer.refreshChart(tableData);
                    }else{
                        executor.shutdown();
                    }
                });
                // 添加窗口关闭监听器，确保资源释放
                signalChartDrawer.addWindowListener(new WindowAdapter() {
                    @Override
                    public void windowClosing(WindowEvent e) {
                        executor.shutdown();
                    }
                });
            }, 0, 200, TimeUnit.MILLISECONDS);
        });

    }

    private void startChartView2(){
        SwingUtilities.invokeLater(() -> {
            ChartContainerForm chartContainerForm = new ChartContainerForm();
            chartContainerForm.setVisible(true);
        });
    }

    private void cleanHandler(ActionEvent e) {
        int result = JOptionPane.showConfirmDialog(
                this,
                "确定要删除该项目下所有已存入数据库中的数据吗？",
                "确认关闭",
                JOptionPane.YES_NO_OPTION,
                JOptionPane.QUESTION_MESSAGE
        );
        System.out.println("result:" + result);
        if (result == JOptionPane.YES_OPTION) {
            // 用户确认关闭，继续默认关闭操作
            doDelProjectData(projectId);
        }
        //enhancedTable.removeAll();
    }

    private void doDelProjectData(Long projectId) {
        DatabaseManager databaseManager = new DatabaseManager();
        databaseManager.execute(CanMessage.class,session -> {
            CanMessageMapper mapper = session.getMapper(CanMessageMapper.class);
            QueryWrapper<CanMessage> wrapper = new QueryWrapper<>();
            TableNameContextHolder.setSuffix("can_message","_p_" + projectId);
            wrapper.lambda().eq(CanMessage::getProjectId,projectId);
            mapper.delete(wrapper);
            TableNameContextHolder.clearSuffix("can_message");
            return null;
        });
    }

    private void reloadHandler(ActionEvent e) {
      
        loadDataByProjectId();
    }

    private void loadBlf(ActionEvent e) {
       
        JFileChooser fileChooser = new JFileChooser();
        fileChooser.setFileFilter(new FileNameExtensionFilter("JSON文件", "json"));
        int returnValue = fileChooser.showOpenDialog(this);
        if (returnValue == JFileChooser.APPROVE_OPTION) {
            File selectedFile = fileChooser.getSelectedFile();
            List<CanMessage> canMessages = BlfParser.parseBlfJson(selectedFile.getAbsolutePath());
            enhancedTable.removeAll();
            enhancedTable.appendBatch(canMessages);
        }
    }

    private static void processAllButtons(Container container) {
        Component[] components = container.getComponents();
        for (Component component : components) {
            if (component instanceof JButton) {
                JButton button = (JButton) component;
                System.out.println("Button:  resize " + button.getText());
                configureButton(button);
            } else if (component instanceof Container) {
                processAllButtons((Container) component);
            }
        }
    }

    private static void configureButton(JButton button) {
        // 设置按钮大小为图标的大小
        Icon icon = button.getIcon();
        if (icon != null) {
            button.setPreferredSize(new Dimension(icon.getIconWidth(), icon.getIconHeight()));
        }

        // 添加悬停效果
        button.addMouseListener(new MouseAdapter() {
            @Override
            public void mouseEntered(MouseEvent e) {
                JButton btn = (JButton) e.getSource();
                btn.setContentAreaFilled(true);
                btn.setOpaque(true);
                btn.setBackground(new Color(230, 230, 230, 100));

            }

            @Override
            public void mouseExited(MouseEvent e) {
                JButton btn = (JButton) e.getSource();
                btn.setContentAreaFilled(false);
                btn.setOpaque(false);
            }
        });
    }

    private void editProject(ActionEvent e) {
        // TODO add your code here
        ProjectFormDialog projectFormDialog = new ProjectFormDialog(null);
        DatabaseManager databaseManager = new DatabaseManager();
        CProject cProject = databaseManager.execute(CProject.class, session -> {
            CProjectMapper mapper = session.getMapper(CProjectMapper.class);
            return mapper.selectById(projectId);
        });
        projectFormDialog.setProject(cProject);
        projectFormDialog.setVisible(true);

    }

    private void readProtobuff(ActionEvent e) {
        // TODO add your code here
//        ErrorInfoDisplay errorInfoDisplay = new ErrorInfoDisplay();
//        errorInfoDisplay.setTitle("读取Protobuf文件");

        // 选文件
        JFileChooser fileChooser = new JFileChooser();
        fileChooser.setFileFilter(new FileNameExtensionFilter("Protobuf文件", "pb"));
        int returnValue = fileChooser.showOpenDialog(this);
        if (returnValue == JFileChooser.APPROVE_OPTION) {
            String largeProtobufPath = fileChooser.getSelectedFile().getAbsolutePath();
            // 新线程中读取大文件
            new Thread(() -> {
                CanProtobufReader canProtobufReader = new CanProtobufReader();
                canProtobufReader.setTable(enhancedTable);
                canProtobufReader.readLargeCanProtobuf(largeProtobufPath, protoMsg -> {
                    CanMessage canEntity = new CanMessage();
                    // 从 Protobuf 消息中提取字段（对应生成文件的 getXxx() 方法）
                    canEntity.setCanId(protoMsg.getCanId());   // 获取 CAN ID（16进制字符串）
                    canEntity.setData(protoMsg.getData());     // 获取数据（16进制字符串）
                    canEntity.setTime(protoMsg.getTime());     // 获取时间戳字符串
                    canEntity.setIndexCounter(CanProtobufReader.count.get());
                });
            }).start();

        }

    }

    private void pagesCombItemStateChanged(ItemEvent e) {
        // TODO add your code here
        if (e.getStateChange() == ItemEvent.SELECTED) {
            // 获取当前选中的项
            String selectedItem = (String) e.getItem();
            // 加载数据
            System.out.println("Selected item: " + selectedItem);
        }
    }

    private void pagesComb(ActionEvent e) {
        // TODO add your code here
    }

    private void loadTxt(ActionEvent e) {
        // TODO add your code here
        LargeFileReader largeFileReader = new LargeFileReader();
        largeFileReader.setProjectId(projectId);
        SwingUtilities.invokeLater(() -> {
            largeFileReader.setVisible(true);
        });
    }

    private void button9(ActionEvent e) {
        // TODO add your code here
        if(sendMultipleConfigDialog ==  null){
            sendMultipleConfigDialog = new SendMessageDialog(null);
            sendMultipleConfigDialog.setProjectId(projectId);
        }
        sendMultipleConfigDialog.setDataReceiver(uppWifiDataReceiver);
        sendMultipleConfigDialog.setVisible(true);
    }

    private void initComponents() {
        // JFormDesigner - Component initialization - DO NOT MODIFY  //GEN-BEGIN:initComponents  @formatter:off
        topPanel = new JPanel();
        label1 = new JLabel();
        label2 = new JLabel();
        channelBox = new JComboBox<>();
        label3 = new JLabel();
        startBtn = new JButton();
        label4 = new JLabel();
        stopBtn = new JButton();
        label5 = new JLabel();
        filterBtn = new JButton();
        label6 = new JLabel();
        button1 = new JButton();
        label7 = new JLabel();
        button2 = new JButton();
        label8 = new JLabel();
        button3 = new JButton();
        label9 = new JLabel();
        button4 = new JButton();
        label10 = new JLabel();
        button7 = new JButton();
        label12 = new JLabel();
        button5 = new JButton();
        label11 = new JLabel();
        button6 = new JButton();
        label17 = new JLabel();
        button8 = new JButton();
        label22 = new JLabel();
        button9 = new JButton();
        hSpacer1 = new JPanel(null);
        panel1 = new JPanel();
        panel2 = new JPanel();
        label13 = new JLabel();
        label14 = new JLabel();
        dataCounter = new JLabel();
        panel3 = new JPanel();
        hSpacer2 = new JPanel(null);
        label19 = new JLabel();
        eveSpeedLabel = new JLabel();
        label21 = new JLabel();
        maxSpeedLabel = new JLabel();
        label16 = new JLabel();
        label20 = new JLabel();
        totalLable = new JLabel();
        label15 = new JLabel();
        pagesComb = new JComboBox<>();
        label18 = new JLabel();
        pageSizeComb = new JComboBox<>();
        tofirstPageBtn = new JButton();
        prePageBtn = new JButton();
        nextPageBtn = new JButton();
        lastPageBtn = new JButton();

        //======== this ========
        setMinimumSize(null);
        setMaximumSize(null);
        setForeground(Color.white);
        setLayout(new BorderLayout());

        //======== topPanel ========
        {
            topPanel.setBorder(new BevelBorder(BevelBorder.LOWERED));
            topPanel.setLayout(new BoxLayout(topPanel, BoxLayout.X_AXIS));

            //---- label1 ----
            label1.setIcon(new ImageIcon(getClass().getResource("/icons/channel.png")));
            label1.setToolTipText("\u9009\u62e9\u901a\u9053");
            topPanel.add(label1);

            //---- label2 ----
            label2.setText("  ");
            topPanel.add(label2);

            //---- channelBox ----
            // 动态加载端口列表（UDP端口 + USB串口）
            channelBox.setModel(new DefaultComboBoxModel<>(getAvailableChannels()));
            channelBox.setMaximumRowCount(10);
            channelBox.setMaximumSize(new Dimension(200, 30));
            channelBox.setMinimumSize(new Dimension(120, 30));
            // 默认选择第二项（跳过刷新选项）
            if (channelBox.getItemCount() > 1) {
                channelBox.setSelectedIndex(1);
            }
            // 添加选择监听器
            channelBox.addActionListener(e -> channelBoxSelectionChanged());
            topPanel.add(channelBox);

            //---- label3 ----
            label3.setText("  ");
            topPanel.add(label3);

            //---- startBtn ----
            startBtn.setIcon(new ImageIcon(getClass().getResource("/icons/start.png")));
            startBtn.setToolTipText("\u5f00\u59cb\u76d1\u542c");
            startBtn.addActionListener(e -> startBtnHandler(e));
            topPanel.add(startBtn);

            //---- label4 ----
            label4.setText("  ");
            topPanel.add(label4);

            //---- stopBtn ----
            stopBtn.setIcon(new ImageIcon(getClass().getResource("/icons/stop.png")));
            stopBtn.setToolTipText("\u505c\u6b62\u76d1\u542c");
            stopBtn.addActionListener(e -> stopBtnHandler(e));
            topPanel.add(stopBtn);

            //---- label5 ----
            label5.setText("  ");
            topPanel.add(label5);

            //---- filterBtn ----
            filterBtn.setIcon(new ImageIcon(getClass().getResource("/icons/filter.png")));
            filterBtn.setToolTipText("\u8fc7\u6ee4");
            topPanel.add(filterBtn);

            //---- label6 ----
            label6.setText("  ");
            topPanel.add(label6);

            //---- button1 ----
            button1.setIcon(new ImageIcon(getClass().getResource("/icons/save.png")));
            button1.setToolTipText("\u4fdd\u5b58\u5230\u6570\u636e\u5e93");
            button1.addActionListener(e -> saveHandler(e));
            topPanel.add(button1);

            //---- label7 ----
            label7.setText("  ");
            topPanel.add(label7);

            //---- button2 ----
            button2.setIcon(new ImageIcon(getClass().getResource("/icons/chart.png")));
            button2.setToolTipText("\u4fe1\u53f7\u66f2\u7ebf\u8ddf\u8e2a");
            button2.addActionListener(e -> trackSignalsHandler(e));
            topPanel.add(button2);

            //---- label8 ----
            label8.setText("  ");
            topPanel.add(label8);

            //---- button3 ----
            button3.setIcon(new ImageIcon(getClass().getResource("/icons/Clear-2.png")));
            button3.setToolTipText("\u6e05\u9664\u8868\u683c\u6570\u636e");
            button3.addActionListener(e -> cleanHandler(e));
            topPanel.add(button3);

            //---- label9 ----
            label9.setText("  ");
            topPanel.add(label9);

            //---- button4 ----
            button4.setIcon(new ImageIcon(getClass().getResource("/icons/reload.png")));
            button4.setToolTipText("\u4ece\u6570\u636e\u5e93\u52a0\u8f7d\u5386\u53f2\u6570\u636e");
            button4.addActionListener(e -> reloadHandler(e));
            topPanel.add(button4);

            //---- label10 ----
            label10.setText("  ");
            topPanel.add(label10);

            //---- button7 ----
            button7.setIcon(new ImageIcon(getClass().getResource("/icons/protobuff.png")));
            button7.addActionListener(e -> readProtobuff(e));
            topPanel.add(button7);

            //---- label12 ----
            label12.setText(" ");
            topPanel.add(label12);

            //---- button5 ----
            button5.setIcon(new ImageIcon(getClass().getResource("/icons/json.png")));
            button5.setToolTipText("\u8bfb\u53d6BLF\u7b49\u6587\u4ef6\u8f6c\u6362\u6765\u7684json\u6570\u636e");
            button5.addActionListener(e -> loadBlf(e));
            topPanel.add(button5);

            //---- label11 ----
            label11.setText("  ");
            topPanel.add(label11);

            //---- button6 ----
            button6.setIcon(new ImageIcon(getClass().getResource("/icons/edit.png")));
            button6.setToolTipText("\u7f16\u8f91\u9879\u76ee\u4fe1\u606f");
            button6.addActionListener(e -> editProject(e));
            topPanel.add(button6);

            //---- label17 ----
            label17.setText(" ");
            topPanel.add(label17);

            //---- button8 ----
            button8.setIcon(new ImageIcon(getClass().getResource("/icons/Txt.png")));
            button8.addActionListener(e -> loadTxt(e));
            topPanel.add(button8);

            //---- label22 ----
            label22.setText(" ");
            topPanel.add(label22);

            //---- button9 ----
            button9.setIcon(new ImageIcon(getClass().getResource("/icons/send.png")));
            button9.addActionListener(e -> button9(e));
            topPanel.add(button9);
            topPanel.add(hSpacer1);
        }
        add(topPanel, BorderLayout.NORTH);

        //======== panel1 ========
        {
            panel1.setLayout(new BoxLayout(panel1, BoxLayout.X_AXIS));

            //======== panel2 ========
            {
                panel2.setLayout(new BoxLayout(panel2, BoxLayout.X_AXIS));

                //---- label13 ----
                label13.setText("\u63a5\u6536\u6570\u636e\u6761\u6570:");
                panel2.add(label13);
                panel2.add(label14);

                //---- dataCounter ----
                dataCounter.setFont(new Font("Inter Semi Bold", Font.ITALIC, 13));
                dataCounter.setForeground(new Color(0x0000cc));
                panel2.add(dataCounter);
            }
            panel1.add(panel2);

            //======== panel3 ========
            {
                panel3.setLayout(new BoxLayout(panel3, BoxLayout.X_AXIS));
                panel3.add(hSpacer2);

                //---- label19 ----
                label19.setText("  \u5e73\u5747\u901f\u7387:");
                panel3.add(label19);

                //---- eveSpeedLabel ----
                eveSpeedLabel.setText("0 Mb/s");
                panel3.add(eveSpeedLabel);

                //---- label21 ----
                label21.setText("  \u6700\u5927\u901f\u7387:");
                panel3.add(label21);

                //---- maxSpeedLabel ----
                maxSpeedLabel.setText("0 Mb/s");
                panel3.add(maxSpeedLabel);

                //---- label16 ----
                label16.setText("  \u603b\u6761\u6570  ");
                panel3.add(label16);
                panel3.add(label20);

                //---- totalLable ----
                totalLable.setText("     ");
                panel3.add(totalLable);

                //---- label15 ----
                label15.setText("  \u5f53\u524d\u9875  ");
                panel3.add(label15);

                //---- pagesComb ----
                pagesComb.setMaximumSize(new Dimension(100, 30));
                pagesComb.setModel(new DefaultComboBoxModel<>(new String[] {
                    "1",
                    "2",
                    "3",
                    "4"
                }));
                pagesComb.addItemListener(e -> pagesCombItemStateChanged(e));
                pagesComb.addActionListener(e -> pagesComb(e));
                panel3.add(pagesComb);

                //---- label18 ----
                label18.setText("pageSize");
                panel3.add(label18);

                //---- pageSizeComb ----
                pageSizeComb.setMaximumSize(new Dimension(84, 30));
                pageSizeComb.setModel(new DefaultComboBoxModel<>(new String[] {
                    "100",
                    "200",
                    "500",
                    "1000",
                    "5000"
                }));
                panel3.add(pageSizeComb);

                //---- tofirstPageBtn ----
                tofirstPageBtn.setText("<<");
                panel3.add(tofirstPageBtn);

                //---- prePageBtn ----
                prePageBtn.setText("<");
                panel3.add(prePageBtn);

                //---- nextPageBtn ----
                nextPageBtn.setText(">");
                panel3.add(nextPageBtn);

                //---- lastPageBtn ----
                lastPageBtn.setText(">>");
                panel3.add(lastPageBtn);
            }
            panel1.add(panel3);
        }
        add(panel1, BorderLayout.SOUTH);
        // JFormDesigner - End of component initialization  //GEN-END:initComponents  @formatter:on
    }

    // JFormDesigner - Variables declaration - DO NOT MODIFY  //GEN-BEGIN:variables  @formatter:off
    private JPanel topPanel;
    private JLabel label1;
    private JLabel label2;
    private JComboBox<String> channelBox;
    private JLabel label3;
    private JButton startBtn;
    private JLabel label4;
    private JButton stopBtn;
    private JLabel label5;
    private JButton filterBtn;
    private JLabel label6;
    private JButton button1;
    private JLabel label7;
    private JButton button2;
    private JLabel label8;
    private JButton button3;
    private JLabel label9;
    private JButton button4;
    private JLabel label10;
    private JButton button7;
    private JLabel label12;
    private JButton button5;
    private JLabel label11;
    private JButton button6;
    private JLabel label17;
    private JButton button8;
    private JLabel label22;
    private JButton button9;
    private JPanel hSpacer1;
    private JPanel panel1;
    private JPanel panel2;
    private JLabel label13;
    private JLabel label14;
    private JLabel dataCounter;
    private JPanel panel3;
    private JPanel hSpacer2;
    private JLabel label19;
    private JLabel eveSpeedLabel;
    private JLabel label21;
    private JLabel maxSpeedLabel;
    private JLabel label16;
    private JLabel label20;
    private JLabel totalLable;
    private JLabel label15;
    private JComboBox<String> pagesComb;
    private JLabel label18;
    private JComboBox<String> pageSizeComb;
    private JButton tofirstPageBtn;
    private JButton prePageBtn;
    private JButton nextPageBtn;
    private JButton lastPageBtn;
    // JFormDesigner - End of variables declaration  //GEN-END:variables  @formatter:on

    private Long projectId;

    private MainFrame parentFrame;
    private EnhancedTable<CanMessage> enhancedTable;
    private static final int MAX_QUEUE_SIZE = 10000;

    public final AtomicBoolean uiUpdateEnabled = new AtomicBoolean(true);

    public final BlockingDeque<byte[]> dataQueue = new LinkedBlockingDeque<>(MAX_QUEUE_SIZE);

    private static final int UI_UPDATE_INTERVAL_MS = 100; // UI更新间隔，单位毫秒

    // 消息缓冲区
    private final List<CanMessage> messageBuffer = new CopyOnWriteArrayList<>();
    // 过滤器
    private ReciverFilterConfigView reciverFilter ;

    // 接收状态
    private boolean isReceiving = false;

    // 接收器
    UdpWifiDataReceiver uppWifiDataReceiver;
    UsbSerialDataReceiver usbSerialDataReceiver;  // USB串口接收器
    
    // 通信模式: "UDP" 或 "USB"
    private String communicationMode = "UDP";

    List<String> targetIds = new ArrayList<>();

    private static final int MAX_PROCESS_ONE_TIME = 500; // 每次最大处理数据量

    // 定时任务
    private ScheduledExecutorService scheduler = Executors.newSingleThreadScheduledExecutor();
    
    /**
     * 添加CAN消息到UI（供USB接收器调用）
     */
    public void addCanMessage(CanMessage message) {
        if (message == null) return;
        
        // 应用过滤器
        if (!targetIds.isEmpty() && !targetIds.contains(message.getCanId())) {
            return;
        }
        
        message.setProjectId(projectId);
        messageBuffer.add(message);
        
        // 如果UI更新启用，触发更新
        if (uiUpdateEnabled.get() && !messageBuffer.isEmpty()) {
            SwingUtilities.invokeLater(this::updateTable);
        }
    }
    
    /**
     * 批量添加CAN消息到UI
     */
    public void addCanMessages(java.util.List<CanMessage> messages) {
        if (messages == null || messages.isEmpty()) return;
        
        for (CanMessage message : messages) {
            if (!targetIds.isEmpty() && !targetIds.contains(message.getCanId())) {
                continue;
            }
            message.setProjectId(projectId);
            messageBuffer.add(message);
        }
        
        if (uiUpdateEnabled.get() && !messageBuffer.isEmpty()) {
            SwingUtilities.invokeLater(this::updateTable);
        }
    }

}
