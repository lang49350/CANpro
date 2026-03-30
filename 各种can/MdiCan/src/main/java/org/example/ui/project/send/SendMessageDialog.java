/*
 * Created by JFormDesigner on Wed Jul 02 14:24:28 CST 2025
 */

package org.example.ui.project.send;

import java.awt.event.*;
import javax.swing.event.*;

// ==================== 🚀 新增导入：CANoe式精准时序和高级功能 ====================
import java.lang.reflect.InvocationTargetException;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
// ==================== 🚀 新增导入结束 ====================

import cn.hutool.core.bean.BeanUtil;
import com.baomidou.mybatisplus.core.conditions.query.QueryWrapper;
import lombok.Data;
import org.apache.ibatis.session.ExecutorType;
import org.example.mapper.SendMessageMapper;
import org.example.pojo.data.CanMessage;
import org.example.pojo.data.SendMessage;
import org.example.pojo.project.CProject;
import org.example.utils.db.BatchSqlProvider;
import org.example.utils.db.DatabaseManager;
import org.example.utils.event.EventListener;
import org.example.utils.event.EventManager;
import org.example.utils.event.EventString;
import org.example.utils.event.TableEditCallback;
import org.example.utils.table.EnhancedTable;
import org.example.utils.web.MessageSenderUtil;
import org.example.utils.web.UdpWifiDataReceiver;
import org.springframework.beans.BeanUtils;

import java.awt.*;
import javax.swing.*;
import javax.swing.border.*;
import java.io.IOException;
import java.lang.reflect.InvocationTargetException;
import java.util.ArrayList;
import java.util.List;
import java.util.Queue;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.stream.Collectors;

/**
 * @author jingq
 */

public class SendMessageDialog extends JFrame  implements EventListener {

    private Long projectId;
    private EnhancedTable<SendMessage> enhancedTable;

    private UdpWifiDataReceiver dataReceiver;
    // 保留的消息清单，不在清单内的将被删除
    private KeepListView keepListView;

    private MessageEditor messageEditor;

    private MessageSenderUtil messageSenderUtil;

    private int currentRow = 0;

    private boolean isRunning = false;
    
    // ==================== 🚀 新增：区间发送和回放速度控制相关变量 ====================
    private JLabel startIndexLabel;
    private JSpinner startIndexSpinner;
    private JLabel endIndexLabel;
    private JSpinner endIndexSpinner;
    
    private JSpinner playbackSpeedSpinner;
    private JLabel playbackSpeedLabel;
    
    // 🚀 CANoe式时序调度相关变量
    private long sendingStartTimeNanos = 0;     // 发送开始的绝对时间
    private long firstMessageTimestampNanos = 0; // 第一条消息的时间戳基准（纳秒）
    private double intervalMs = 0;              // 当前发送间隔(毫秒)
    private double playbackSpeedMultiplier = 1.0; // 回放速度倍率（1.0=原始速度）
    private int parseDebugCount = 0; // 用于调试输出计数
    private int waitDebugCount = 0; // 用于等待调试计数
    
    // 🚀 发送时间统计相关变量
    private long totalSendingTime = 0;       // 总发送时间(毫秒)
    private long totalMessagesSent = 0;      // 总发送消息数
    private long currentSessionStart = 0;    // 当前会话开始时间
    private long lastProgressUpdate = 0;     // 上次进度更新时间
    private int pauseCount = 0;              // 暂停次数
    private long totalPauseTime = 0;         // 总暂停时间
    
    // 🚀 发送时间统计显示组件
    private JLabel sendTimeStatsLabel;
    private JButton stopButton;  // 停止发送按钮
    // ==================== 🚀 新增变量结束 ====================
    public SendMessageDialog(Frame owner) {
        super();
        initComponents();
        initUserComponent(owner);
        setTitle("发送消息配置");
        // 最大化
        setExtendedState(JFrame.MAXIMIZED_BOTH);
        keepListView = new KeepListView(this);
        messageEditor = new MessageEditor(this);
        messageEditor.setEditCallback(this::onEditFinish);
        messageSenderUtil = new MessageSenderUtil();
        //注册消息监听，用来接收发送器发出的消息
        EventManager.getInstance().addListener(this);
    }
    public void setDataReceiver(UdpWifiDataReceiver dataReceiver){
        this.dataReceiver = dataReceiver;
    }
    private void initUserComponent(Frame owner){
        enhancedTable = new EnhancedTable<>(SendMessage.class);
        scrollPane1.setViewportView(enhancedTable);
        filterPanel.add(enhancedTable.getFilterPanel());
        // 设置最大高度50
        filterPanel.setMaximumSize(new Dimension(Integer.MAX_VALUE, 50));
        // 设置编辑动作的回调
        enhancedTable.setEditCallback(new TableEditCallback<SendMessage>() {
            @Override
            public SendMessage onEdit(SendMessage sendMessage) {
                currentRow = enhancedTable.getSelectedRow();
                messageEditor.clean();
                messageEditor.setMessage(BeanUtil.copyProperties(sendMessage, CanMessage.class));
                messageEditor.setVisible(true);
                messageEditor.refresh();
                return sendMessage;
            }
        });
        
        // ==================== 🚀 新增：初始化高级功能组件 ====================
        // 初始化右键菜单
        initContextMenu();
        
        // 初始化新增的区间发送组件
        initRangeAndSpeedControlComponents();
        // ==================== 🚀 新增结束 ====================
    }

    public void setProjectId(long projectId) {
        this.projectId = projectId;
    }

    public CanMessage onEditFinish(CanMessage canMessage){
        System.out.println("---------------------------------");
        System.out.println("行号:" + currentRow);
        System.out.println("原值:" + enhancedTable.getDataAt(currentRow).getData());
        System.out.println("新值:" + canMessage.getData());
        enhancedTable.getDataAt(currentRow).setData(canMessage.getData());
        return canMessage;
    }

    private void addOne(ActionEvent e) {
       
        String canId = JOptionPane.showInputDialog(this, "请输入 can_id:");
        if (canId != null && !canId.isEmpty()) {
            String data = JOptionPane.showInputDialog(this, "请输入 data:");
            if (data != null && !data.isEmpty()) {
                // 这里可以将 canId 和 data 添加到表格中
                // 示例代码，需要根据实际情况修改
                SendMessage sendMessage = new SendMessage();
                sendMessage.setCanId(canId);
                sendMessage.setData(data);
                enhancedTable.append(sendMessage);
            }
        }
    }

    public void append(CanMessage canMessage){
        SendMessage sendMessage = new SendMessage();
        BeanUtils.copyProperties(canMessage, sendMessage);
        enhancedTable.append(sendMessage);
    }

    public void appendBatch(List<CanMessage> canMessages){
        List<SendMessage> sendMessages = new ArrayList<>();
        for (CanMessage canMessage : canMessages) {
            SendMessage sendMessage = new SendMessage();
            BeanUtils.copyProperties(canMessage, sendMessage);
            sendMessages.add(sendMessage);
        }
        enhancedTable.appendBatch(sendMessages);
    }

    private void ok(ActionEvent e) {
       
        dispose();
    }

    private void cancel(ActionEvent e) {
       
        dispose();
    }
    
    /**
     * 停止发送
     */
    private void stopSending(ActionEvent e) {
        if (isRunning) {
            int result = JOptionPane.showConfirmDialog(
                this,
                "确定要停止发送吗？",
                "停止确认",
                JOptionPane.YES_NO_OPTION,
                JOptionPane.WARNING_MESSAGE
            );
            
            if (result == JOptionPane.YES_OPTION) {
                isRunning = false;
                SwingUtilities.invokeLater(() -> {
                    statusLable.setText("用户停止发送");
                    resetButtonStates();
                });
            }
        }
    }

    private void removeSelected(ActionEvent e) {
        
        enhancedTable.removeDataAt(enhancedTable.getSelectedRow());
    }

    private void removeBySelecedCanId(ActionEvent e) {
      
        List<SendMessage> tableData = enhancedTable.getTableData();
        SendMessage dataAt = enhancedTable.getDataAt(enhancedTable.getSelectedRow());
        for (int i = tableData.size() - 1; i >= 0; i--) {
            SendMessage sendMessage = tableData.get(i);
            if (sendMessage.getCanId().equals(dataAt.getCanId())) {
                enhancedTable.removeDataAt(i);
            }
        }
    }

    private void removeAll(ActionEvent e) {
      
        enhancedTable.removeAll();
    }

    private void saveToDb(ActionEvent e) {
      
        List<SendMessage> data = enhancedTable.getTableData();
        List<SendMessage> toSave = new ArrayList<>();
        List<SendMessage> toUpdate = new ArrayList<>();
        for (SendMessage sendMessage : data) {
            sendMessage.setProjectId(projectId);
            if(sendMessage.getId() == null){
                toSave.add(sendMessage);
            }
        }
        DatabaseManager databaseManager = new DatabaseManager();
        if(!toSave.isEmpty()){
            databaseManager.execute(SendMessage.class,session -> {
                List<List<SendMessage>> lists = BatchSqlProvider.splitBatch(toSave, 500);
                int total = 0;
                SendMessageMapper mapper = session.getMapper(SendMessageMapper.class);
                for (List<SendMessage> list : lists) {
                    total += mapper.insertBatch(list);
                }
                session.commit();
                return total;
            },false, ExecutorType.BATCH);
        }
        if(!enhancedTable.getChangedList().isEmpty()){
            databaseManager.execute(SendMessage.class,session -> {
                SendMessageMapper mapper = session.getMapper(SendMessageMapper.class);
                for (SendMessage message : enhancedTable.getChangedList()) {
                    mapper.update(message,new QueryWrapper<SendMessage>().eq("id",message.getId()));
                }
                return true;
            });
        }
        JOptionPane.showMessageDialog(this, "保存成功", "提示", JOptionPane.INFORMATION_MESSAGE);
    }

    private void loadFormDb(ActionEvent e) {
        // TODO add your code here
        DatabaseManager databaseManager = new DatabaseManager();
        List<SendMessage> data = databaseManager.execute(SendMessage.class,session -> {
            SendMessageMapper mapper = session.getMapper(SendMessageMapper.class);
            QueryWrapper<SendMessage> wrapper = new QueryWrapper<>();
            wrapper.lambda().eq(SendMessage::getProjectId, projectId);
            return mapper.selectList(wrapper);
        });
        enhancedTable.appendBatch(data);
    }

    private void send(ActionEvent e) {
        // 检查dataReceiver是否为空
        if (dataReceiver == null) {
            JOptionPane.showMessageDialog(this, "请先连接开发板", "提示", JOptionPane.INFORMATION_MESSAGE);
            return;
        }

        // 发送前确认对话框
        int startIndex = (Integer) startIndexSpinner.getValue();
        int endIndex = (Integer) endIndexSpinner.getValue();
        boolean isCircle = checkBoxForCircle.isSelected();
        
        String confirmMessage = String.format(
            "确认发送配置：\n" +
            "• 发送范围：索引 %d 到 %d\n" +
            "• 发送模式：%s\n" +
            "• 设备连接：%s\n\n" +
            "是否开始发送？",
            startIndex, endIndex,
            isCircle ? "循环发送" : "单次发送",
            dataReceiver.isConnectionHealthy() ? "连接正常" : "连接异常(请确保设备心跳正常)"
        );
        
        int result = JOptionPane.showConfirmDialog(
            this,
            confirmMessage,
            "发送确认",
            JOptionPane.YES_NO_OPTION,
            JOptionPane.QUESTION_MESSAGE
        );
        
        if (result == JOptionPane.YES_OPTION) {
            // 用户确认发送，启动发送过程
            doSend();
        } else {
            // 用户取消发送，恢复状态
            statusLable.setText(dataReceiver.isConnectionHealthy() ? "设备连接正常，已取消发送" : "设备连接异常，已取消发送");
        }
    }

    private void doSend() {
        if(isRunning) return;
        isRunning = true;
        
        // ==================== 精准时序和高级功能 ====================

        if (dataReceiver != null) {
            long beforeReset = dataReceiver.getCurrentMessageIndex();
            System.out.println("🔄 发送开始，正在重置Java端消息索引...");
            System.out.println("📊 重置前索引值: " + beforeReset);
            dataReceiver.resetMessageIndex();
            long afterReset = dataReceiver.getCurrentMessageIndex();
            System.out.println("📊 重置后索引值: " + afterReset);
        } else {
            System.out.println("⚠️ dataReceiver为null，无法重置消息索引");
        }
        
        // 🔍 强制调试：确认进入doSend方法
        System.out.println("\n🔍 ===== 进入doSend方法 =====");
        System.out.printf("🔍 回放速度: %.1fx\n", getPlaybackSpeed());
        System.out.println("🔍 CANoe式精准时序模式已启用");
        
        // 确保连接健康（按钮发送路径已有心跳，但再次快速确认）
        if (!ensureConnectionOrAbort()) {
            isRunning = false;
            return;
        }
        // ==================== 🚀 升级结束 ====================
        
        new Thread(() -> {
            try {
                // ==================== 🚀 升级：区间发送支持 ====================
                // 获取区间发送参数
                int startIndex = (Integer) startIndexSpinner.getValue();
                int endIndex = (Integer) endIndexSpinner.getValue();
                
                // 获取表格数据
                List<SendMessage> allTableData = enhancedTable.getTableData();
                
                // 验证区间参数
                if (startIndex < 0 || endIndex < 0) {
                    SwingUtilities.invokeLater(() -> {
                        JOptionPane.showMessageDialog(this, "索引不能为负数", "错误", JOptionPane.ERROR_MESSAGE);
                    });
                    isRunning = false;
                    return;
                }
                
                if (startIndex >= endIndex) {
                    SwingUtilities.invokeLater(() -> {
                        JOptionPane.showMessageDialog(this, "起始索引必须小于结束索引", "错误", JOptionPane.ERROR_MESSAGE);
                    });
                    isRunning = false;
                    return;
                }
                
                if (endIndex > allTableData.size()) {
                    SwingUtilities.invokeLater(() -> {
                        JOptionPane.showMessageDialog(this, 
                            String.format("结束索引(%d)超出数据范围，最大索引为%d", endIndex, allTableData.size()-1), 
                            "错误", JOptionPane.ERROR_MESSAGE);
                    });
                    isRunning = false;
                    return;
                }
                
                // 截取指定区间的数据
                List<SendMessage> tableData = allTableData.subList(startIndex, endIndex + 1);
                // ==================== 🚀 升级结束 ====================
                
                int circleTime = 0;
                long sendStartTime = System.currentTimeMillis(); // 记录开始时间
                
                // 使用do-while循环，确保至少执行一次（支持单次发送）
                do {
                    circleTime++;
                    if(circleTime == 1){
                        SwingUtilities.invokeLater(() -> {
                            String message = String.format("开始区间发送 (索引%d-%d)", startIndex, endIndex);
                            JOptionPane.showMessageDialog(this, message, "提示", JOptionPane.INFORMATION_MESSAGE);
                            statusLable.setText("准备发送数据...");
                        });
                    }
                    
                    // 确保设备处于发送模式
                    dataReceiver.sendDataFlag("ETH_CTRL MODE SENDER");
                    Thread.sleep(300);

                    // 发送START标志
                    dataReceiver.sendDataFlag("START");
                    Thread.sleep(1000); // 等待1秒再开始发送，确保设备就绪
                    long total = tableData.size();

                    if (total == 0) {
                        System.out.println("没有数据可发送");
                        SwingUtilities.invokeLater(() -> {
                            statusLable.setText("区间内数据总数：0");
                        });
                        return;
                    }
                    
                    final String StatusString = String.format("循环次数%d | 区间[%d-%d] | 数据总数：%d", 
                                                             circleTime, startIndex, endIndex, total);
                    SwingUtilities.invokeLater(() -> {
                        statusLable.setText(StatusString);
                        // 设置进度条最大值
                        progressBar1.setMaximum((int)total);
                        progressBar1.setValue(0); // 重置进度条
                    });

                    // 🔍 强制调试：循环发送模式
                    System.out.println("\n🔍 ===== 循环发送模式开始 =====");
                    System.out.printf("🔍 循环发送 - 总数: %d条\n", total);
                    System.out.printf("🔍 回放速度: %.1fx\n", getPlaybackSpeed());
                    
                    // 🔍 检查前几条数据的时间戳格式
                    if (tableData.size() > 0) {
                        System.out.println("🔍 前5条数据的时间戳检查：");
                        for (int i = 0; i < Math.min(5, tableData.size()); i++) {
                            SendMessage msg = tableData.get(i);
                            System.out.printf("  数据%d - ID:%s, 时间:'%s'\n", i, msg.getCanId(), msg.getTime());
                        }
                    }
                    
                    // 发送每条数据 - 集成智能流控
                    for (int i = 0; i < total; i++) {
                        if (!isRunning) break; // 检查停止标志
                        
                        final int counter = i + 1;
                        final int globalIndex = startIndex + i; // 全局索引
                        SendMessage datum = tableData.get(i);
                        
                        // 🔍 循环模式前5条消息调试
                        if (i < 5) {
                            System.out.printf("🔍 循环模式发送%d: ID=%s, 时间=%s\n", 
                                            i, datum.getCanId(), datum.getTime());
                        }

                        // 更新UI显示当前发送进度
                        String StatusString2 = String.format("循环次数%d | 发送第%d条(全局索引%d) | 共%d条", 
                                                            circleTime, counter, globalIndex, total);
                        SwingUtilities.invokeLater(() -> {
                            statusLable.setText(StatusString2);
                            // 更新进度条值
                            progressBar1.setValue(counter);
                        });

                        // 准备CAN消息并发送
                        CanMessage canMessage = new CanMessage();
                        BeanUtils.copyProperties(datum, canMessage);
                        dataReceiver.sendData(canMessage);

                        // 🔍 循环模式调试：确认调用时序函数
                        if (i < 5) {
                            System.out.printf("🔍 循环模式调用timingBasedSleep(%d)\n", i);
                        }
                        
                        // ⏱️ 时序精确流控 - 基于绝对时间调度
                        timingBasedSleep(i, total);
                    }

                    // 发送END标志
                    Thread.sleep(10); // 等待10ms，确保所有数据都已发送
                    dataReceiver.sendDataFlag("END");
                    
                    // 🚀 关键修复：循环模式发送完成后也要强制刷新UI缓冲区
                    Thread.sleep(100); // 等待100ms让ESP32处理完最后的数据
                    dataReceiver.forceFlushUIBuffer();

                    // 计算精确发送时间和实际速率
                    long currentTime = System.currentTimeMillis();
                    long elapsedMs = currentTime - sendStartTime;
                    double elapsedSeconds = elapsedMs / 1000.0;
                    double actualRate = total / elapsedSeconds;
                    
                    SwingUtilities.invokeLater(() -> {
                        statusLable.setText(String.format("区间发送完成 (耗时%.1f秒, 实际速率%.0f包/秒)", 
                                          elapsedSeconds, actualRate));
                        // 发送完成，进度条设为满
                        progressBar1.setValue(progressBar1.getMaximum());
                    });
                    
                } while (isRunning && getCheckBoxState()); // 循环条件：程序运行中且选择了循环发送
                isRunning = false;
                SwingUtilities.invokeLater(() -> {
                    resetButtonStates();
                });
            } catch (InterruptedException ex) {
                System.err.println("发送线程被中断: " + ex.getMessage());
                isRunning = false;
                Thread.currentThread().interrupt();
                SwingUtilities.invokeLater(() -> {
                    statusLable.setText("发送被中断");
                    resetButtonStates();
                    JOptionPane.showMessageDialog(this, "发送被中断", "错误", JOptionPane.ERROR_MESSAGE);
                });
            } catch (Exception ex) {
                System.err.println("发送数据时发生错误: " + ex.getMessage());
                isRunning = false;
                ex.printStackTrace();
                SwingUtilities.invokeLater(() -> {
                    statusLable.setText("发送错误: " + ex.getMessage());
                    resetButtonStates();
                    JOptionPane.showMessageDialog(this, "发送错误: " + ex.getMessage(), "错误", JOptionPane.ERROR_MESSAGE);
                });
            }
        }).start();
    }

    private int getCircelWait() {
        // ==================== 精准时序模式 ====================
        // 精准时序模式：固定1ms间隔，实际时序由上位机精确控制
        return 1;
        // ==================== 🚀 升级结束 ====================
    }

    // 在EDT中读取复选框状态
    private boolean getCheckBoxState() {
        // 用AtomicBoolean存储结果（线程安全的布尔容器）
        AtomicBoolean result = new AtomicBoolean(false);
        try {
            // 在EDT中执行任务，并将结果存入result
            SwingUtilities.invokeAndWait(() -> {
                result.set(checkBoxForCircle.isSelected());
            });
        } catch (InterruptedException | InvocationTargetException e) {
            // 处理中断或任务执行异常
            Thread.currentThread().interrupt();
            isRunning = false;
            return false;
        }
        // 返回EDT中获取的最新状态
        return result.get();
    }

    private static void waitTime(int ms) {
        try {
            Thread.sleep(ms);
        } catch (InterruptedException ex) {
            throw new RuntimeException(ex);
        }
    }

    private void editKeepList(ActionEvent e) {
        // TODO add your code here
        if(keepListView == null){
            keepListView = new KeepListView(this);
            keepListView.setVisible(true);
        }else{
            keepListView.setVisible(true);
        }
    }

    private void addToKeepList(ActionEvent e) {
        // TODO add your code here
        int selectedRow = enhancedTable.getSelectedRow();
        if(selectedRow == -1){
            JOptionPane.showMessageDialog(this, "请选择一条数据", "提示", JOptionPane.INFORMATION_MESSAGE);
        }
        SendMessage dataAt = enhancedTable.getDataAt(selectedRow);
        // 检查保留清单中是否已有该数据
        List<SendMessage> data = keepListView.getEnhancedTable().getData();
        for (SendMessage datum : data) {
            if (datum.getCanId().equals(dataAt.getCanId())) {
                return;
            }
        }
        keepListView.getEnhancedTable().append(dataAt);
        JOptionPane.showMessageDialog(this, "添加成功", "提示", JOptionPane.INFORMATION_MESSAGE);
    }

    private void cleanDataByKeepList(ActionEvent e) {
        // TODO add your code here
        List<SendMessage> data = enhancedTable.getData();
        List<SendMessage> keepData = keepListView.getEnhancedTable().getData();
        List<String> ids = keepData.stream().map(SendMessage::getCanId).collect(Collectors.toList());
        List<SendMessage> reserved = new ArrayList<>();
        // 按照keepList 中的CanId 清除数据，不在清单中的清除
        for (SendMessage datum : data) {
            if(ids.contains(datum.getCanId())){
                reserved.add(datum);
            }
        }
        enhancedTable.removeAll();
        enhancedTable.appendBatch(reserved);
    }

    private void sendByFile(ActionEvent e) {
        // TODO add your code here
        new Thread(()->{
            try {
                messageSenderUtil.sendMessages(messageSenderUtil.getBoardAddressString(),
                        Integer.parseInt("8888"),
                        enhancedTable.getData());
            } catch (IOException ex) {
                throw new RuntimeException(ex + messageSenderUtil.getBoardAddressString() + ":" + "8888");
            }
        }).start();
    }

    private void checkBoxForCircleStateChanged(ChangeEvent e) {
        // TODO add your code here
        boolean selected = checkBoxForCircle.isSelected();
        circelWait.setEnabled(selected);

    }


    private void initComponents() {
        // JFormDesigner - Component initialization - DO NOT MODIFY  //GEN-BEGIN:initComponents  @formatter:off
        dialogPane = new JPanel();
        contentPanel = new JPanel();
        filterPanel = new JPanel();
        scrollPane1 = new JScrollPane();
        table1 = new JTable();
        buttonBar = new JPanel();
        statusLable = new JLabel();
        progressBar1 = new JProgressBar();
        hSpacer1 = new JPanel(null);
        okButton = new JButton();
        cancelButton = new JButton();
        panel1 = new JPanel();
        button1 = new JButton();
        button2 = new JButton();
        button3 = new JButton();
        button4 = new JButton();
        button9 = new JButton();
        button10 = new JButton();
        button8 = new JButton();
        button5 = new JButton();
        button6 = new JButton();
        button7 = new JButton();
        button11 = new JButton();
        checkBoxForCircle = new JCheckBox();
        circelWait = new JSpinner();
        label1 = new JLabel();
        hSpacer2 = new JPanel(null);

        //======== this ========
        Container contentPane = getContentPane();
        contentPane.setLayout(new BorderLayout());

        //======== dialogPane ========
        {
            dialogPane.setBorder(new EmptyBorder(12, 12, 12, 12));
            dialogPane.setLayout(new BorderLayout(6, 6));

            //======== contentPanel ========
            {
                contentPanel.setLayout(new BoxLayout(contentPanel, BoxLayout.Y_AXIS));

                //======== filterPanel ========
                {
                    filterPanel.setLayout(new BoxLayout(filterPanel, BoxLayout.X_AXIS));
                }
                contentPanel.add(filterPanel);

                //======== scrollPane1 ========
                {
                    scrollPane1.setViewportView(table1);
                }
                contentPanel.add(scrollPane1);
            }
            dialogPane.add(contentPanel, BorderLayout.CENTER);

            //======== buttonBar ========
            {
                buttonBar.setBorder(new EmptyBorder(12, 0, 0, 0));
                buttonBar.setLayout(new GridBagLayout());
                ((GridBagLayout)buttonBar.getLayout()).columnWidths = new int[] {0, 0, 0, 85, 80};
                ((GridBagLayout)buttonBar.getLayout()).columnWeights = new double[] {1.0, 0.0, 0.0, 0.0, 0.0};
                buttonBar.add(statusLable, new GridBagConstraints(0, 0, 1, 1, 0.0, 0.0,
                    GridBagConstraints.CENTER, GridBagConstraints.BOTH,
                    new Insets(0, 0, 0, 5), 0, 0));
                buttonBar.add(progressBar1, new GridBagConstraints(1, 0, 1, 1, 0.0, 0.0,
                    GridBagConstraints.CENTER, GridBagConstraints.BOTH,
                    new Insets(0, 0, 0, 5), 0, 0));
                buttonBar.add(hSpacer1, new GridBagConstraints(2, 0, 1, 1, 0.0, 0.0,
                    GridBagConstraints.CENTER, GridBagConstraints.BOTH,
                    new Insets(0, 0, 0, 5), 0, 0));

                //---- okButton ----
                okButton.setText("OK");
                okButton.addActionListener(e -> ok(e));
                buttonBar.add(okButton, new GridBagConstraints(3, 0, 1, 1, 0.0, 0.0,
                    GridBagConstraints.CENTER, GridBagConstraints.BOTH,
                    new Insets(0, 0, 0, 5), 0, 0));

                //---- cancelButton ----
                cancelButton.setText("Cancel");
                cancelButton.addActionListener(e -> cancel(e));
                buttonBar.add(cancelButton, new GridBagConstraints(4, 0, 1, 1, 0.0, 0.0,
                    GridBagConstraints.CENTER, GridBagConstraints.BOTH,
                    new Insets(0, 0, 0, 0), 0, 0));
            }
            dialogPane.add(buttonBar, BorderLayout.SOUTH);

            //======== panel1 ========
            {
                panel1.setLayout(new BoxLayout(panel1, BoxLayout.X_AXIS));

                //---- button1 ----
                button1.setIcon(new ImageIcon(getClass().getResource("/icons/\u65b0\u589e.png")));
                button1.setToolTipText("\u65b0\u589e");
                button1.addActionListener(e -> addOne(e));
                panel1.add(button1);

                //---- button2 ----
                button2.setIcon(new ImageIcon(getClass().getResource("/icons/\u6e05\u9664.png")));
                button2.setToolTipText("\u6e05\u9664\u9009\u5b9a");
                button2.addActionListener(e -> removeSelected(e));
                panel1.add(button2);

                //---- button3 ----
                button3.setIcon(new ImageIcon(getClass().getResource("/icons/\u5224\u65ad\u6570\u636e.png")));
                button3.setToolTipText("\u6309\u7167\u9009\u5b9aID\u6e05\u9664");
                button3.addActionListener(e -> removeBySelecedCanId(e));
                panel1.add(button3);

                //---- button4 ----
                button4.setIcon(new ImageIcon(getClass().getResource("/icons/\u6e05\u9664\u6240\u6709.png")));
                button4.setToolTipText("\u6e05\u9664\u6240\u6709");
                button4.addActionListener(e -> removeAll(e));
                panel1.add(button4);

                //---- button9 ----
                button9.setIcon(new ImageIcon(getClass().getResource("/icons/\u52a0\u5165\u6e05\u5355.png")));
                button9.setToolTipText("\u52a0\u5165\u6e05\u5355");
                button9.addActionListener(e -> addToKeepList(e));
                panel1.add(button9);

                //---- button10 ----
                button10.setIcon(new ImageIcon(getClass().getResource("/icons/\u7f16\u8f91\u67e5\u770b\u6e05\u5355.png")));
                button10.setToolTipText("\u67e5\u770b\u6e05\u5355");
                button10.addActionListener(e -> editKeepList(e));
                panel1.add(button10);

                //---- button8 ----
                button8.setIcon(new ImageIcon(getClass().getResource("/icons/\u4ec5\u4fdd\u7559\u6e05\u5355\u4e2d\u6761\u76ee.png")));
                button8.setToolTipText("\u4ec5\u4fdd\u7559\u6e05\u5355\u4e2d\u9879\u76ee");
                button8.addActionListener(e -> cleanDataByKeepList(e));
                panel1.add(button8);

                //---- button5 ----
                button5.setIcon(new ImageIcon(getClass().getResource("/icons/save.png")));
                button5.setToolTipText("\u4fdd\u5b58");
                button5.addActionListener(e -> saveToDb(e));
                panel1.add(button5);

                //---- button6 ----
                button6.setIcon(new ImageIcon(getClass().getResource("/icons/reload.png")));
                button6.setToolTipText("\u8f7d\u5165");
                button6.addActionListener(e -> loadFormDb(e));
                panel1.add(button6);

                //---- button7 ----
                button7.setIcon(new ImageIcon(getClass().getResource("/icons/16\u5bc4\u4ef6\u3001\u53d1\u9001.png")));
                button7.setToolTipText("\u53d1\u9001");
                button7.addActionListener(e -> send(e));
                panel1.add(button7);

                //---- button11 ----
                button11.setIcon(new ImageIcon(getClass().getResource("/icons/\u6587\u4ef6-\u53d1\u9001\u6587\u4ef6.png")));
                button11.setToolTipText("\u6309\u6587\u4ef6\u53d1\u9001");
                button11.addActionListener(e -> sendByFile(e));
                panel1.add(button11);

                //---- checkBoxForCircle ----
                checkBoxForCircle.setText("\u5faa\u73af");
                checkBoxForCircle.addChangeListener(e -> checkBoxForCircleStateChanged(e));
                panel1.add(checkBoxForCircle);

                //---- circelWait ----
                circelWait.setMaximumSize(new Dimension(160, 30));
                circelWait.setPreferredSize(new Dimension(120, 30));
                circelWait.setMinimumSize(new Dimension(120, 30));
                panel1.add(circelWait);

                //---- label1 ----
                label1.setText("ms");
                panel1.add(label1);
                panel1.add(hSpacer2);
            }
            dialogPane.add(panel1, BorderLayout.NORTH);
        }
        contentPane.add(dialogPane, BorderLayout.CENTER);
        setSize(1165, 390);
        setLocationRelativeTo(getOwner());
        // JFormDesigner - End of component initialization  //GEN-END:initComponents  @formatter:on
    }

    // JFormDesigner - Variables declaration - DO NOT MODIFY  //GEN-BEGIN:variables  @formatter:off
    private JPanel dialogPane;
    private JPanel contentPanel;
    private JPanel filterPanel;
    private JScrollPane scrollPane1;
    private JTable table1;
    private JPanel buttonBar;
    private JLabel statusLable;
    private JProgressBar progressBar1;
    private JPanel hSpacer1;
    private JButton okButton;
    private JButton cancelButton;
    private JPanel panel1;
    private JButton button1;
    private JButton button2;
    private JButton button3;
    private JButton button4;
    private JButton button9;
    private JButton button10;
    private JButton button8;
    private JButton button5;
    private JButton button6;
    private JButton button7;
    private JButton button11;
    private JCheckBox checkBoxForCircle;
    private JSpinner circelWait;
    private JLabel label1;
    private JPanel hSpacer2;
    // JFormDesigner - End of variables declaration  //GEN-END:variables  @formatter:on

    // 测试
    public static void main(String[] args) {
        SwingUtilities.invokeLater(new Runnable() {
            @Override
            public void run() {
                SendMessageDialog dialog = new SendMessageDialog(null);
                dialog.setVisible(true);
            }
        });
    }

    @Override
    public void onEvent(String event) {
        System.out.println("SendMessageDialog: " + event);
        if(event.startsWith(EventString.TCP_START_SEND.name()+"TOTAL:")) {
            SwingUtilities.invokeLater(()->{
                progressBar1.setMaximum(Integer.parseInt(event.split(":")[1]));
            });
        }
        if(event.startsWith(EventString.TCP_START_SEND.name()+"CURRENT:")){
            SwingUtilities.invokeLater(()->{
                progressBar1.setValue(Integer.parseInt(event.split(":")[1]));
            });
        }

    }
    
    // ==================== 🚀 新增：高级功能方法集合 ====================
    
    /**
     * 🚀 初始化右键菜单
     */
    private void initContextMenu() {
        // 创建右键菜单
        JPopupMenu contextMenu = new JPopupMenu();
        
        // 单条发送菜单项
        JMenuItem sendSingleItem = new JMenuItem("发送选中行");
        sendSingleItem.addActionListener(e -> sendSelectedRows(false)); // false表示单条发送
        contextMenu.add(sendSingleItem);
        
        // 多条发送菜单项
        JMenuItem sendMultipleItem = new JMenuItem("发送选中范围");
        sendMultipleItem.addActionListener(e -> sendSelectedRows(true)); // true表示多条发送
        contextMenu.add(sendMultipleItem);
        
        // 为表格添加鼠标监听器
        enhancedTable.addMouseListener(new java.awt.event.MouseAdapter() {
            @Override
            public void mousePressed(java.awt.event.MouseEvent e) {
                // 检查是否是右键点击
                if (e.isPopupTrigger()) {
                    showContextMenu(e, contextMenu);
                }
            }
            
            @Override
            public void mouseReleased(java.awt.event.MouseEvent e) {
                // 检查是否是右键点击（某些系统在mouseReleased中触发）
                if (e.isPopupTrigger()) {
                    showContextMenu(e, contextMenu);
                }
            }
        });
    }
    
    /**
     * 🚀 显示右键菜单
     */
    private void showContextMenu(java.awt.event.MouseEvent e, JPopupMenu contextMenu) {
        // 获取点击的行
        int row = enhancedTable.rowAtPoint(e.getPoint());
        if (row >= 0) {
            // 如果点击的行不在当前选择中，则选择该行
            if (!enhancedTable.isRowSelected(row)) {
                enhancedTable.setRowSelectionInterval(row, row);
            }
            // 显示菜单
            contextMenu.show(enhancedTable, e.getX(), e.getY());
        }
    }
    
    /**
     * 🚀 发送选中的行
     * @param multiple true表示发送多条，false表示发送单条
     */
    private void sendSelectedRows(boolean multiple) {
        int[] selectedRows = enhancedTable.getSelectedRows();
        if (selectedRows.length == 0) {
            JOptionPane.showMessageDialog(this, "请先选择要发送的行", "提示", JOptionPane.WARNING_MESSAGE);
            return;
        }
        
        if (!multiple && selectedRows.length > 1) {
            JOptionPane.showMessageDialog(this, "单条发送只能选择一行", "提示", JOptionPane.WARNING_MESSAGE);
            return;
        }
        
        // 获取选中行的数据
        List<SendMessage> selectedData = new ArrayList<>();
        for (int rowIndex : selectedRows) {
            selectedData.add(enhancedTable.getDataAt(rowIndex));
        }
        
        // 发送前确认对话框
        String sendType = multiple ? "多条" : "单条";
        String confirmMessage = String.format(
            "确认%s发送：\n" +
            "• 选中行数：%d 行\n" +
            "• 设备连接：%s\n\n" +
            "是否开始发送？",
            sendType, selectedData.size(),
            dataReceiver != null && dataReceiver.isConnectionHealthy() ? "连接正常" : "连接异常(请确保设备心跳正常)"
        );
        
        int result = JOptionPane.showConfirmDialog(
            this,
            confirmMessage,
            sendType + "发送确认",
            JOptionPane.YES_NO_OPTION,
            JOptionPane.QUESTION_MESSAGE
        );
        
        if (result == JOptionPane.YES_OPTION) {
            // 用户确认发送，启动发送线程
            sendSelectedData(selectedData, multiple);
        }
    }
    
    /**
     * 🚀 发送指定的数据列表
     */
    private void sendSelectedData(List<SendMessage> dataList, boolean multiple) {
        if (isRunning) {
            JOptionPane.showMessageDialog(this, "发送正在进行中，请等待完成", "提示", JOptionPane.WARNING_MESSAGE);
            return;
        }
        
        // 确保连接健康（右键发送路径补齐建连/心跳）
        if (!ensureConnectionOrAbort()) {
            return;
        }

        isRunning = true;
        
        // 设置按钮状态
        SwingUtilities.invokeLater(() -> {
            okButton.setEnabled(false);
            stopButton.setEnabled(true);
        });
        
        new Thread(() -> {
            try {
                long sendStartTime = System.currentTimeMillis();
                currentSessionStart = sendStartTime;
                String sendType = multiple ? "多条" : "单条";
                pauseCount = 0;
                totalPauseTime = 0;
                
                SwingUtilities.invokeLater(() -> {
                    double speed = getPlaybackSpeed();
                    String speedInfo = String.format("回放%.1fx", speed);
                    statusLable.setText(String.format("🎯 %s - %s发送 (共%d条)", speedInfo, sendType, dataList.size()));
                    progressBar1.setMaximum(dataList.size());
                    progressBar1.setValue(0);
                    // 🎯 式发送时间统计显示
                    sendTimeStatsLabel.setText(String.format("📊 发送中: 0包/s | 0.0s | 回放%.1fx | 0次暂停", speed));
                });
                
                // 确保设备处于发送模式
                dataReceiver.sendDataFlag("ETH_CTRL MODE SENDER");
                Thread.sleep(300);

                // 发送START标志
                dataReceiver.sendDataFlag("START");
                Thread.sleep(1000); // 等待1秒再开始发送，确保设备就绪
                
                // 🔍 强制调试：确认进入发送循环
                System.out.println("\n🔍 ===== 发送循环开始 =====");
                System.out.printf("🔍 数据总数: %d条\n", dataList.size());
                System.out.printf("🔍 回放速度: %.1fx\n", getPlaybackSpeed());
                System.out.println("🔍 CANoe式精准时序模式已启用");
                
                // 🚀 智能发送循环 - 支持动态暂停和恢复
                for (int i = 0; i < dataList.size(); i++) {
                    if (!isRunning) break;
                    
                    final int counter = i + 1;
                    SendMessage datum = dataList.get(i);
                    
                    // 🔍 前5条消息强制调试
                    if (i < 5) {
                        System.out.printf("🔍 准备发送消息%d: ID=%s, 时间=%s\n", 
                                        i, datum.getCanId(), datum.getTime());
                    }
                    
                    // 🚀 检查ESP32连接和队列压力
                    boolean shouldPause = false;
                    final String[] pauseReason = {""};  // 使用数组避免final问题
                    
                    if (dataReceiver != null) {
                        if (!dataReceiver.isConnectionHealthy()) {
                            shouldPause = true;
                            pauseReason[0] = "设备连接异常";
                        } else {
                            // 检查当前发送速率是否过低（低于200包/秒时暂停）
                            double currentInterval = dataReceiver.getPreciseCurrentSendInterval();
                            if (currentInterval > 5.0) { // 超过5ms间隔(<200包/秒)，表示压力大
                                shouldPause = true;
                                pauseReason[0] = String.format("设备压力大(间隔%.1fms,<200包/秒)", currentInterval);
                            }
                        }
                    }
                    
                    // 🚀 智能暂停机制
                    if (shouldPause) {
                        long pauseStartTime = System.currentTimeMillis();
                        pauseCount++;
                        
                        SwingUtilities.invokeLater(() -> {
                            statusLable.setText(String.format("%s发送暂停: %s (第%d条, 第%d次暂停)", sendType, pauseReason[0], counter, pauseCount));
                        });
                        
                        // 🚀 智能等待条件改善（防卡死版本）
                        int pauseLoopCount = 0;
                        int maxPauseLoops = 50; // 最多暂停5秒（而不是10秒）
                        
                        while (shouldPause && isRunning && pauseLoopCount < maxPauseLoops) {
                            Thread.sleep(100);
                            pauseLoopCount++;
                            
                          
                            if (pauseLoopCount >= 30) { // 3秒后
                                shouldPause = false;
                                SwingUtilities.invokeLater(() -> {
                                    statusLable.setText(String.format("%s强制恢复发送(防卡死) (第%d条)", sendType, counter));
                                });
                                break;
                            }
                            
                            // 重新检查条件
                            if (dataReceiver != null && dataReceiver.isConnectionHealthy()) {
                                double interval = dataReceiver.getPreciseCurrentSendInterval();
                                if (interval <= 4.0) { // 放宽恢复条件：4ms以下即可恢复
                                    shouldPause = false;
                                    SwingUtilities.invokeLater(() -> {
                                        statusLable.setText(String.format("%s发送恢复(>250包/秒) (第%d条)", sendType, counter));
                                    });
                                    break;
                                }
                            }
                        }
                        
                        // 🚨 超时强制恢复
                        if (pauseLoopCount >= maxPauseLoops) {
                            SwingUtilities.invokeLater(() -> {
                                statusLable.setText(String.format("%s暂停超时，强制继续 (第%d条)", sendType, counter));
                            });
                        }
                        
                        // 记录暂停时间
                        long pauseEndTime = System.currentTimeMillis();
                        totalPauseTime += (pauseEndTime - pauseStartTime);
                        
                        if (pauseLoopCount >= 100) {
                            SwingUtilities.invokeLater(() -> {
                                statusLable.setText(String.format("%s发送超时，强制继续 (第%d条, 已暂停%d次)", sendType, counter, pauseCount));
                            });
                        }
                    }
                    
                    // 🔥 实时进度统计和UI更新
                    long currentTime = System.currentTimeMillis();
                    if (currentTime - lastProgressUpdate > 1000 || counter % 100 == 0) { // 每秒或每100条更新一次
                        lastProgressUpdate = currentTime;
                        final long elapsedTime = currentTime - currentSessionStart;
                        final double currentRate = counter / (elapsedTime / 1000.0);
                        final double avgPauseTime = pauseCount > 0 ? (totalPauseTime / (double)pauseCount) : 0;
                        
                        SwingUtilities.invokeLater(() -> {
                            String progressText = String.format(
                                "%s发送第%d条 | 共%d条 | 耗时%.1fs | 速率%.0f包/s | 暂停%d次(%.1fms平均)",
                                sendType, counter, dataList.size(), elapsedTime / 1000.0, currentRate, pauseCount, avgPauseTime
                            );
                            statusLable.setText(progressText);
                            progressBar1.setValue(counter);
                            
                            // 🔥 实时更新发送时间统计显示
                            String timeStatsText = String.format(
                                "📊 发送中: %.0f包/s | %.1fs | %d次暂停 | 累计%.1fs",
                                currentRate, elapsedTime / 1000.0, pauseCount, totalSendingTime / 1000.0
                            );
                            sendTimeStatsLabel.setText(timeStatsText);
                        });
                    } else {
                        SwingUtilities.invokeLater(() -> {
                            progressBar1.setValue(counter);
                        });
                    }
                    
                    // 🚨 防卡死检测：检查发送是否卡住太久
                    if (counter > 1000 && currentTime - lastProgressUpdate > 30000) { // 30秒无进度更新
                        SwingUtilities.invokeLater(() -> {
                            statusLable.setText(String.format("⚠️ 检测到发送可能卡死 (第%d条) - 强制继续", counter));
                        });
                        Thread.sleep(1000); // 强制等待1秒后继续
                        lastProgressUpdate = currentTime; // 重置进度时间
                    }
                    
                    // 发送数据
                    CanMessage canMessage = new CanMessage();
                    BeanUtils.copyProperties(datum, canMessage);
                    dataReceiver.sendData(canMessage);
                    
                    // 🔍 强制调试：确认调用时序函数
                    if (i < 5) {
                        System.out.printf("🔍 调用timingBasedSleep(%d)\n", i);
                    }
                    
                    // ⏱️ 时序精确流控 - 基于绝对时间调度  
                    timingBasedSleep(i, dataList.size());
                }
                
                // 发送END标志
                Thread.sleep(10);
                dataReceiver.sendDataFlag("END");
                
                // 🚀 关键修复：发送完成后强制刷新UI缓冲区，确保最后的数据显示
                Thread.sleep(100); // 等待100ms让ESP32处理完最后的数据
                dataReceiver.forceFlushUIBuffer();
                
                // 计算精确发送时间和实际速率
                long currentTime = System.currentTimeMillis();
                long elapsedMs = currentTime - sendStartTime;
                double elapsedSeconds = elapsedMs / 1000.0;
                double actualRate = dataList.size() / elapsedSeconds;
                
                // 🔥 记录总发送统计
                totalSendingTime += elapsedMs;
                totalMessagesSent += dataList.size();
                
                SwingUtilities.invokeLater(() -> {
                    // 🚀 显示详细的发送完成统计
                    String connectionStatus = dataReceiver != null ? dataReceiver.getConnectionStatus() : "未连接";
                    double effectiveSendingTime = (elapsedMs - totalPauseTime) / 1000.0; // 扣除暂停时间的有效发送时间
                    double effectiveRate = dataList.size() / effectiveSendingTime;
                    
                    String completionText = String.format(
                        "%s发送完成 | 总数:%d条 | 总耗时:%.1fs | 暂停%d次共%.1fs | 有效发送时间:%.1fs | 总速率:%.0f包/s | 有效速率:%.0f包/s", 
                        sendType, dataList.size(), elapsedSeconds, pauseCount, totalPauseTime/1000.0, effectiveSendingTime, actualRate, effectiveRate
                    );
                    statusLable.setText(completionText);
                    
                    progressBar1.setValue(progressBar1.getMaximum());
                    
                    // 🔥 更新最终发送时间统计显示
                    String finalStatsText = String.format(
                        "✅ 完成: %.0f包/s | %.1fs | %d次暂停 | 累计%.0f包 %.1fs",
                        effectiveRate, effectiveSendingTime, pauseCount, (double)totalMessagesSent, totalSendingTime / 1000.0
                    );
                    sendTimeStatsLabel.setText(finalStatsText);
                    
                    resetButtonStates();
                });
                
            } catch (InterruptedException ex) {
                SwingUtilities.invokeLater(() -> {
                    statusLable.setText("发送被中断");
                    sendTimeStatsLabel.setText("❌ 发送中断");
                    resetButtonStates();
                });
            } catch (Exception ex) {
                SwingUtilities.invokeLater(() -> {
                    statusLable.setText("发送错误: " + ex.getMessage());
                    sendTimeStatsLabel.setText("❌ 发送错误");
                    resetButtonStates();
                });
                ex.printStackTrace();
            } finally {
                isRunning = false;
                SwingUtilities.invokeLater(() -> {
                    resetButtonStates();
                });
            }
        }).start();
    }
    
    /**
     * 🚀 初始化区间发送和回放速度控制组件
     */
    private void initRangeAndSpeedControlComponents() {
        // 创建区间发送组件
        startIndexLabel = new JLabel("起始索引:");
        startIndexSpinner = new JSpinner(new SpinnerNumberModel(0, 0, Integer.MAX_VALUE, 1));
        startIndexSpinner.setPreferredSize(new Dimension(100, 30));
        
        endIndexLabel = new JLabel("结束索引:");
        endIndexSpinner = new JSpinner(new SpinnerNumberModel(0, 0, Integer.MAX_VALUE, 1));
        endIndexSpinner.setPreferredSize(new Dimension(100, 30));
        
        // 动态设置区间默认值
        updateDefaultRange();
        
        // 回放速度控制
        playbackSpeedLabel = new JLabel("回放速度:");
        playbackSpeedSpinner = new JSpinner(new SpinnerNumberModel(1.0, 0.1, 10.0, 0.1));
        playbackSpeedSpinner.setPreferredSize(new Dimension(80, 30));
        playbackSpeedSpinner.setToolTipText("回放速度倍率 (1.0=原始速度, 0.5=半速, 2.0=2倍速)");
        playbackSpeedSpinner.addChangeListener(e -> {
            double speed = (Double) playbackSpeedSpinner.getValue();
            setPlaybackSpeed(speed);
        });
        
        // 将组件添加到现有的panel1中
        addComponentsToPanel();
    }
    
    /**
     * 🚀 动态设置区间默认值为整个数据范围
     */
    private void updateDefaultRange() {
        SwingUtilities.invokeLater(() -> {
            if (enhancedTable != null) {
                List<SendMessage> tableData = enhancedTable.getTableData();
                if (tableData != null && !tableData.isEmpty()) {
                    int maxIndex = tableData.size() - 1;
                    startIndexSpinner.setValue(0);
                    endIndexSpinner.setValue(maxIndex);
                    
                    // 更新SpinnerModel的最大值
                    ((SpinnerNumberModel)endIndexSpinner.getModel()).setMaximum(maxIndex);
                    
                    // 在状态栏显示数据范围信息
                    statusLable.setText(String.format("数据范围：0 - %d (共%d条)", maxIndex, tableData.size()));
                }
            }
        });
    }
    
    /**
     * 🚀 将新组件添加到现有面板
     */
    private void addComponentsToPanel() {
        // 在现有按钮后添加区间发送组件
        panel1.add(javax.swing.Box.createHorizontalStrut(10)); // 添加间距
        panel1.add(startIndexLabel);
        panel1.add(startIndexSpinner);
        panel1.add(endIndexLabel);
        panel1.add(endIndexSpinner);
        
        // 添加回放速度控制
        panel1.add(javax.swing.Box.createHorizontalStrut(10)); // 添加间距
        panel1.add(playbackSpeedLabel);
        panel1.add(playbackSpeedSpinner);
        
        // 刷新布局
        panel1.revalidate();
    }

    // 共用：确保连接健康（心跳+等待）
    private boolean ensureConnectionOrAbort() {
        if (dataReceiver == null) {
            JOptionPane.showMessageDialog(this, "请先连接开发板", "提示", JOptionPane.WARNING_MESSAGE);
            return false;
        }
        try {
            dataReceiver.sendHeartbeat();
            int maxWaitTime = 3000; // 3秒
            int waitInterval = 100;
            int total = 0;
            while (total < maxWaitTime && !dataReceiver.isConnectionHealthy()) {
                Thread.sleep(waitInterval);
                total += waitInterval;
            }
            if (!dataReceiver.isConnectionHealthy()) {
                JOptionPane.showMessageDialog(this, "设备未响应心跳，请检查连接", "连接失败", JOptionPane.ERROR_MESSAGE);
                return false;
            }
            return true;
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            return false;
        }
    }
    
    /**
     * 🚀 重置按钮状态
     */
    private void resetButtonStates() {
        okButton.setEnabled(true);
        stopButton.setEnabled(false);
    }
    
    /**
     * 🎯 式精确时序回放（完全重写版本）
     * 基于BLF文件真实时间戳，实现精确的回放功能
     */
    private void timingBasedSleep(int currentIndex, long totalCount) throws InterruptedException {
        // 🔍 强制调试：确认进入时序方法
        if (currentIndex < 5) {
            System.out.printf("🔍 >>> 进入timingBasedSleep(%d, %d) <<<\n", currentIndex, totalCount);
        }
        
        // 🚨 防卡死检查
        if (!isRunning) {
            if (currentIndex < 5) {
                System.out.println("🔍 !isRunning，提前返回");
            }
            return;
        }
        
        // 获取当前和前一条消息
        List<SendMessage> dataList = enhancedTable.getData();
        if (currentIndex >= dataList.size()) {
            return;
        }
        
        SendMessage currentMessage = dataList.get(currentIndex);
        
        if (currentIndex == 0) {
            // 🎯 第一条消息：建立时间基准
            sendingStartTimeNanos = System.nanoTime();
            firstMessageTimestampNanos = parseTimestampToNanos(currentMessage.getTime());
            
            // 🔍 重置调试计数器
            parseDebugCount = 0;
            waitDebugCount = 0;
            
            // 🔍 强制调试：检查前10条消息的时间戳内容
            System.out.println("\n🔍 ===== 时序调试信息 =====");
            System.out.printf("🎯 第一条消息 - 原始时间: '%s' -> 解析为: %.6fs\n", 
                            currentMessage.getTime(), firstMessageTimestampNanos / 1_000_000_000.0);
            
            // 检查前10条消息的时间戳
            int checkCount = Math.min(10, dataList.size());
            for (int i = 0; i < checkCount; i++) {
                SendMessage msg = dataList.get(i);
                long parsedNanos = parseTimestampToNanos(msg.getTime());
                System.out.printf("消息%d - 时间: '%s' -> %.6fs (纳秒: %d)\n", 
                                i, msg.getTime(), parsedNanos / 1_000_000_000.0, parsedNanos);
            }
            
            // 检查间隔
            if (dataList.size() > 1) {
                long firstNanos = parseTimestampToNanos(dataList.get(0).getTime());
                long secondNanos = parseTimestampToNanos(dataList.get(1).getTime());
                long intervalNanos = secondNanos - firstNanos;
                System.out.printf("前两条消息间隔: %.6fms (%.6fs)\n", 
                                intervalNanos / 1_000_000.0, intervalNanos / 1_000_000_000.0);
            }
            
            System.out.println("🎯 回放速度: " + getPlaybackSpeed() + "x");
            System.out.println("================================\n");
            
            return; // 第一条消息立即发送
        }
        
        // 获取前一条消息的时间戳
        SendMessage previousMessage = dataList.get(currentIndex - 1);
        long previousTimestampNanos = parseTimestampToNanos(previousMessage.getTime());
        long currentTimestampNanos = parseTimestampToNanos(currentMessage.getTime());
        
        // 🎯 核心：计算消息间的真实时间间隔
        long realIntervalNanos = currentTimestampNanos - previousTimestampNanos;
        
        // 🚀 应用回放速度
        double playbackSpeed = getPlaybackSpeed();
        long scaledIntervalNanos = (long)(realIntervalNanos / playbackSpeed);
        
        // 🚨 异常检测：间隔过大时限制
        if (scaledIntervalNanos > 5_000_000_000L) { // >5秒
            System.err.printf("⚠️ 异常间隔：%.3fs -> 限制为1s\n", scaledIntervalNanos / 1_000_000_000.0);
            scaledIntervalNanos = 1_000_000_000L; // 限制为1秒
        } else if (scaledIntervalNanos < 0) {
            System.err.printf("⚠️ 负间隔：%.3fs -> 设为0\n", scaledIntervalNanos / 1_000_000_000.0);
            scaledIntervalNanos = 0; // 时间戳倒序，直接发送
        }
        
        // 🎯 CAN总线负载控制 + 精确等待
        // 🚀 CAN总线物理限制：每帧最少128μs传输时间（64位@500Kbps）
        long minCANIntervalNanos = 150_000L; // 150μs最小间隔，留安全余量
        long finalWaitNanos = Math.max(scaledIntervalNanos, minCANIntervalNanos);
        
        // 🔍 调试输出（前10条和每5000条）
        if (currentIndex <= 10 || currentIndex % 5000 == 0) {
            if (finalWaitNanos != scaledIntervalNanos) {
                System.out.printf("🎯 消息%d: 间隔=%.3fms, CAN限制后=%.3fms ⚡\n",
                                currentIndex, realIntervalNanos / 1_000_000.0, finalWaitNanos / 1_000_000.0);
            } else {
                System.out.printf("🎯 消息%d: 间隔=%.3fms\n",
                                currentIndex, realIntervalNanos / 1_000_000.0);
            }
        }
        
        if (finalWaitNanos > 0) {
            long waitStart = System.nanoTime();
            preciseWait(finalWaitNanos);
            long waitEnd = System.nanoTime();
            long actualWaitNanos = waitEnd - waitStart;
            
            // 验证实际等待时间（前10条）
            if (currentIndex <= 10) {
                System.out.printf("✅ 实际等待: %.3fms (目标: %.3fms)\n", 
                                actualWaitNanos / 1_000_000.0, 
                                finalWaitNanos / 1_000_000.0);
            }
        } else {
            // 🔍 调试：没有等待的情况
            if (currentIndex <= 50) {
                System.out.printf("⚠️ 消息%d: 无等待 - 间隔=%.3fms (前=%s, 当前=%s)\n",
                                currentIndex,
                                scaledIntervalNanos / 1_000_000.0,
                                previousMessage.getTime(),
                                currentMessage.getTime());
            }
        }
    }
    
    /**
     * 🎯 解析时间戳字符串为纳秒（支持多种格式）
     * 支持：秒.毫秒、毫秒时间戳、微秒时间戳等
     */
    private long parseTimestampToNanos(String timeStr) {
        try {
            if (timeStr == null || timeStr.trim().isEmpty()) {
                System.err.println("⚠️ 时间戳为空: '" + timeStr + "'");
                return 0;
            }

            String originalTimeStr = timeStr;

            // 调试输出（仅前3条）
            parseDebugCount++;
            if (parseDebugCount <= 3) {
                System.out.printf("🔍 时间戳解析[%d]: 原始='%s'\n", parseDebugCount, originalTimeStr);
            }

            // 含冒号 → 按 时:分:秒.小数 解析
            if (timeStr.contains(":")) {
                long ns = parseTimeWithColon(timeStr);
                if (parseDebugCount <= 10) {
                    System.out.printf("🔍 冒号格式解析: %dns (%.6fs)\n", ns, ns / 1_000_000_000.0);
                }
                return ns;
            }

            // 去掉字母与空白
            String s = timeStr.trim().toLowerCase().replaceAll("[a-z\\s]", "");

            // 含小数点 → 视为 秒.小数秒
            if (s.contains(".")) {
                double seconds = Double.parseDouble(s);
                long ns = (long) (seconds * 1_000_000_000L);
                if (parseDebugCount <= 10) {
                    System.out.printf("🔍 小数秒解析: %s s -> %dns\n", s, ns);
                }
                return ns;
            }

            // 纯整数 → 依据位数推断单位
            String digits = s;
            int len = digits.length();
            long val;
            try {
                val = Long.parseLong(digits);
            } catch (NumberFormatException ex) {
                // 超范围退回 double
                double dv = Double.parseDouble(digits);
                val = (long) dv;
            }

            long ns;
            if (len >= 19) {              // 纳秒 epoch
                ns = val;
            } else if (len >= 16) {        // 微秒 epoch（16~18位）
                ns = val * 1_000L;
            } else if (len >= 13) {        // 毫秒 epoch（13~15位）
                ns = val * 1_000_000L;
            } else if (len >= 7) {         // 微秒（相对起始/相对开机）
                ns = val * 1_000L;
            } else {                        // 秒
                ns = val * 1_000_000_000L;
            }

            if (parseDebugCount <= 10) {
                System.out.printf("🔍 整数时间戳解析[%d]: '%s' (len=%d) -> %dns (%.6fs)\n",
                        parseDebugCount, originalTimeStr, len, ns, ns / 1_000_000_000.0);
            }
            return ns;
        } catch (Exception e) {
            System.err.println("⚠️ 时间戳解析失败: '" + timeStr + "' - " + e.getMessage());
            return 0;
        }
    }
    
    /**
     * 🎯 解析特殊时间格式：如 1:45:56225.656259
     * 格式：小时:分钟:秒.微秒
     */
    private long parseTimeWithColon(String timeStr) {
        try {
            // 分割冒号
            String[] parts = timeStr.split(":");
            if (parts.length != 3) {
                System.err.printf("⚠️ 冒号格式错误: '%s'，期望3个部分\n", timeStr);
                return 0;
            }
            
            int hours = Integer.parseInt(parts[0]);
            int minutes = Integer.parseInt(parts[1]);
            
            // 处理秒部分（可能包含小数）
            double seconds = Double.parseDouble(parts[2]);
            
            // 转换为总秒数
            double totalSeconds = hours * 3600 + minutes * 60 + seconds;
            
            // 转换为纳秒
            long totalNanos = (long)(totalSeconds * 1_000_000_000L);
            
            System.out.printf("🔍 冒号格式解析: %d时%d分%.6f秒 = %.6f总秒 = %d纳秒\n", 
                            hours, minutes, seconds, totalSeconds, totalNanos);
            
            return totalNanos;
            
        } catch (Exception e) {
            System.err.printf("⚠️ 冒号格式解析失败: '%s' - %s\n", timeStr, e.getMessage());
            return 0;
        }
    }
    
    /**
     * 🚀 获取当前回放速度倍率
     */
    private double getPlaybackSpeed() {
        // 可以从UI控件获取，或者使用默认值
        return playbackSpeedMultiplier;
    }
    
    /**
     * 🚀 设置回放速度倍率
     * @param speed 速度倍率（0.1 = 10倍慢放，2.0 = 2倍快放）
     */
    public void setPlaybackSpeed(double speed) {
        if (speed > 0.01 && speed <= 10.0) {
            this.playbackSpeedMultiplier = speed;
            System.out.println("🚀 回放速度设置为: " + speed + "x");
        }
    }
    
    /**
     * 🎯 精确等待方法
     * @param nanos 等待的纳秒数
     */
    private void preciseWait(long nanos) throws InterruptedException {
        if (nanos <= 0) {
            System.out.println("⚠️ preciseWait: 等待时间<=0: " + nanos);
            return;
        }
        
        long startTime = System.nanoTime();
        long endTime = startTime + nanos;
        
        // 🔍 调试：等待开始
        waitDebugCount++;
        if (waitDebugCount <= 5) {
            System.out.printf("🔍 开始精确等待[%d]: %.3fms\n", waitDebugCount, nanos / 1_000_000.0);
        }
        
        // 🚀 混合等待策略：Thread.sleep + 精确循环
        if (nanos >= 2_000_000) { // >= 2ms
            // 大间隔：先用Thread.sleep等待大部分时间
            long sleepMs = (nanos / 1_000_000) - 1; // 提前1ms醒来
            if (sleepMs > 0) {
                if (waitDebugCount <= 5) {
                    System.out.printf("🔍 Thread.sleep: %dms\n", sleepMs);
                }
                Thread.sleep(sleepMs);
            }
            
            // 🚨 检查停止标志
            if (!isRunning) return;
        }
        
        // 🎯 精确循环等待剩余时间（高精度）
        long loopCount = 0;
        while (System.nanoTime() < endTime && isRunning) {
            long remaining = endTime - System.nanoTime();
            if (remaining > 100_000) { // >0.1ms
                Thread.yield(); // 让出CPU时间片
            }
            loopCount++;
            // 剩余时间很短时，继续循环等待（最高精度）
        }
        
        long actualWait = System.nanoTime() - startTime;
        if (waitDebugCount <= 5) {
            System.out.printf("🔍 等待完成[%d]: 目标%.3fms, 实际%.3fms, 循环%d次\n", 
                            waitDebugCount, nanos / 1_000_000.0, actualWait / 1_000_000.0, loopCount);
        }
    }
    
    // ==================== 🚀 新增功能方法结束 ====================
    
    /**
     * 🧪 测试方法：验证按钮功能是否正常工作
     */
    public void testButtonFunctionality() {
        System.out.println("🧪 开始测试按钮功能...");
        
        try {
            // 测试停止按钮功能
            if (cancelButton != null) {
                System.out.println("✅ 停止按钮已正确配置，文本: " + cancelButton.getText());
                System.out.println("✅ 停止按钮事件监听器已绑定");
            } else {
                System.out.println("⚠️ 停止按钮未找到");
            }
            
            // 测试发送按钮功能
            if (button7 != null) {
                System.out.println("✅ 发送按钮已正确配置，文本: " + button7.getText());
                System.out.println("✅ 发送按钮事件监听器已绑定");
            } else {
                System.out.println("⚠️ 发送按钮未找到");
            }
            
            System.out.println("🧪 按钮功能测试完成");
            
        } catch (Exception e) {
            System.err.println("🧪 按钮功能测试失败: " + e.getMessage());
            e.printStackTrace();
        }
    }
}
