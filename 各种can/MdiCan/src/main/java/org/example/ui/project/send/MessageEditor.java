/*
 * Created by JFormDesigner on Mon Jul 14 13:15:01 CST 2025
 */

package org.example.ui.project.send;

import cn.hutool.core.bean.BeanUtil;
import com.baomidou.mybatisplus.core.conditions.query.QueryWrapper;
import org.apache.commons.lang3.StringUtils;
import org.example.mapper.CProjectMapper;
import org.example.mapper.DbcMessageMapper;
import org.example.mapper.DbcSignalMapper;
import org.example.pojo.data.CanMessage;
import org.example.pojo.dbc.DbcMessage;
import org.example.pojo.dbc.DbcSignal;
import org.example.pojo.dbc.vo.DbcSignalVo;
import org.example.pojo.project.CProject;
import org.example.utils.db.DatabaseManager;
import org.example.utils.db.ExplanCanData;
import org.example.utils.event.TableEditCallback;
import org.example.utils.table.EnhancedTable;
import org.springframework.beans.BeanUtils;

import java.awt.*;
import java.awt.event.*;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import javax.swing.*;
import javax.swing.border.*;
import javax.swing.event.*;

/**
 * @author jingq
 */
public class MessageEditor extends JFrame {
    private EnhancedTable<DbcSignalVo> enhancedTable;

    private TableEditCallback<CanMessage> editCallback;

    private DbcMessage dbcMessage;

    private List<String> unshowCols = Arrays.asList("userComment","comment");
    public MessageEditor(Window owner) {
        super();
        setTitle("消息编辑");
        initComponents();
        init();
    }

    public void setEditCallback(TableEditCallback<CanMessage> editCallback){
        this.editCallback = editCallback;
    }

    public void clean(){
        enhancedTable.removeAll();
    }

    private void init() {
        enhancedTable = new EnhancedTable<>(DbcSignalVo.class);
        enhancedTable.setUseVerboserName(true);
        //设置不显示字段
        enhancedTable.setUnShowColumns(unshowCols);
        // 刷新表格结构
        enhancedTable.initColumns();
        JScrollPane scrollPane = new JScrollPane(enhancedTable);
        centerPanel.add(scrollPane, BorderLayout.CENTER);

        // 定义表格编辑事件
        enhancedTable.setEnableEditCheck(true);
        enhancedTable.setEditCheckCallback(this::checkEditVal);
    }
    private void checkEditVal(String columnName, int row ,int col,Object oldVal,Object newVal){
        DbcSignalVo dataAt = enhancedTable.getDataAt(row);
        String maximum = dataAt.getMaximum();
        String minimum = dataAt.getMinimum();
        // 转换为double 后比较 newVal 是否超限制
        if(StringUtils.isNotBlank(maximum) && Double.parseDouble(newVal.toString()) > Double.parseDouble(maximum)){
            JOptionPane.showMessageDialog(this, "当前值超过最大值限制");
        }
        if(StringUtils.isNotBlank(minimum) && Double.parseDouble(newVal.toString()) < Double.parseDouble(minimum)){
            JOptionPane.showMessageDialog(this, "当前值小于最小值限制");
        }
    }

    /**
     * 刷新数据
     */
    public void refresh(){
        DatabaseManager dataBaseManager = new DatabaseManager();
        // 获取项目信息
        CProject cProject = dataBaseManager.execute(CProject.class, session -> {
            CProjectMapper mapper = session.getMapper(CProjectMapper.class);
            return mapper.selectById(canMessage.getProjectId());
        });
        System.out.println(cProject);
        if(cProject.getDbcMetadataId() == null){
            JOptionPane.showMessageDialog(this, "项目未绑定dbc文件，请先绑定dbc文件");
            return;
        }
        // 根据dbcId获取获取信号
        String canId = canMessage.getCanId();
        // 转0x000002A6格式
        String formattedHex = String.format("0x%08X", Integer.parseInt(canId, 16));
        List<DbcSignal> signals = dataBaseManager.execute(DbcSignal.class, session -> {
            DbcSignalMapper mapper = session.getMapper(DbcSignalMapper.class);
            QueryWrapper<DbcSignal> wrapper = new QueryWrapper<>();
            wrapper.lambda().eq(DbcSignal::getDbcMetadataId, cProject.getDbcMetadataId())
                    // 因为CanMessage的消息Id不带0x.不带前置的0,DbcSignal中的消息Id带0x，所以这里需要转换一下
            .eq(DbcSignal::getMessageId, formattedHex);
            return mapper.selectList(wrapper);
        });

        // 设置消息头信息
        DbcMessage message = dataBaseManager.execute(DbcMessage.class, (session) -> {
            DbcMessageMapper mapper = session.getMapper(DbcMessageMapper.class);
            QueryWrapper<DbcMessage> wrapper = new QueryWrapper<>();
            wrapper.lambda().eq(DbcMessage::getDbcMetadataId, cProject.getDbcMetadataId())
                    .eq(DbcMessage::getMessageId, formattedHex);
            return mapper.selectOne(wrapper);
        });
        dbcMessage = message;
        // 上面的信息设置
        messageName.setText(message.getName());
        messageId.setText(formattedHex);
        cycleTime.setText(message.getCycleTime());
        sender.setText(message.getSender());
        messageComment.setText(message.getComment());
        userMessageComment.setText(message.getUserComment());
        dataLength.setText(String.valueOf(message.getLength()));
        sendType.setText(message.getSendType());
        List<DbcSignalVo> dbcSignalVos = BeanUtil.copyToList(signals, DbcSignalVo.class);
        refreshBySignals(dbcSignalVos);

    }

    private void refreshBySignals(List<DbcSignalVo> signals) {
        signals.forEach(signal -> {

            double value = ExplanCanData.extractSignalValue(ExplanCanData.hexStringToByteArray(canMessage.getData()),
                    BeanUtil.copyProperties(signal, DbcSignal.class));
            signal.setCurrentValue(String.valueOf(value));
            signal.setValue(String.valueOf(value));
        });

        enhancedTable.appendBatch(signals);
    }

    private void onOk(ActionEvent e) {
        // TODO add your code here
        List<DbcSignalVo> tableData = enhancedTable.getTableData();
        byte[] bytes =
                ExplanCanData.generateCanMessageData(tableData, Integer.parseInt(dbcMessage.getLength()));
        StringBuilder hexString = new StringBuilder();
        for (byte b : bytes) {
            System.out.printf("%02X ", b);
            hexString.append(String.format("%02X", b));
        }
        canMessage.setData(hexString.toString());
        editCallback.onEdit(canMessage);
        dispose();
    }

    private void onCancel(ActionEvent e) {
        // TODO add your code here
        dispose();
    }

    private void showCommonCheckBoxStateChanged(ChangeEvent e) {
        // TODO add your code here
        if(showCommonCheckBox.isSelected()){
            enhancedTable.setUnShowColumns(new ArrayList<>());
            enhancedTable.initColumns();
        }else{
            enhancedTable.setUnShowColumns(unshowCols);
            enhancedTable.initColumns();
        }


    }

    public void setMessage(CanMessage canMessage) {
        this.canMessage = canMessage;
    }

    private void initComponents() {
        // JFormDesigner - Component initialization - DO NOT MODIFY  //GEN-BEGIN:initComponents  @formatter:off
        dialogPane = new JPanel();
        contentPanel = new JPanel();
        panel1 = new JPanel();
        topPanel = new JPanel();
        label1 = new JLabel();
        messageId = new JLabel();
        label2 = new JLabel();
        messageName = new JLabel();
        label3 = new JLabel();
        dataLength = new JLabel();
        label4 = new JLabel();
        sendType = new JLabel();
        label5 = new JLabel();
        cycleTime = new JLabel();
        label6 = new JLabel();
        sender = new JLabel();
        topPanel2 = new JPanel();
        label7 = new JLabel();
        messageComment = new JTextField();
        label8 = new JLabel();
        userMessageComment = new JTextField();
        centerPanel = new JPanel();
        panel2 = new JPanel();
        showCommonCheckBox = new JCheckBox();
        saveBtn = new JButton();
        button2 = new JButton();
        button3 = new JButton();

        //======== this ========
        Container contentPane = getContentPane();
        contentPane.setLayout(new BorderLayout());

        //======== dialogPane ========
        {
            dialogPane.setBorder(new EmptyBorder(12, 12, 12, 12));
            dialogPane.setLayout(new BorderLayout());

            //======== contentPanel ========
            {
                contentPanel.setLayout(new BorderLayout());

                //======== panel1 ========
                {
                    panel1.setLayout(new BoxLayout(panel1, BoxLayout.Y_AXIS));

                    //======== topPanel ========
                    {
                        topPanel.setLayout(new FlowLayout(FlowLayout.LEFT, 10, 5));

                        //---- label1 ----
                        label1.setText("\u6d88\u606fID:");
                        topPanel.add(label1);

                        //---- messageId ----
                        messageId.setText("text");
                        topPanel.add(messageId);

                        //---- label2 ----
                        label2.setText("\u6d88\u606f\u540d\u79f0:");
                        topPanel.add(label2);

                        //---- messageName ----
                        messageName.setText("text");
                        topPanel.add(messageName);

                        //---- label3 ----
                        label3.setText("\u6570\u636e\u957f\u5ea6\uff1a");
                        topPanel.add(label3);

                        //---- dataLength ----
                        dataLength.setText("text");
                        topPanel.add(dataLength);

                        //---- label4 ----
                        label4.setText("\u53d1\u9001\u7c7b\u578b:");
                        topPanel.add(label4);

                        //---- sendType ----
                        sendType.setText("text");
                        topPanel.add(sendType);

                        //---- label5 ----
                        label5.setText("\u5468\u671f:");
                        topPanel.add(label5);

                        //---- cycleTime ----
                        cycleTime.setText("text");
                        topPanel.add(cycleTime);

                        //---- label6 ----
                        label6.setText("\u53d1\u9001\u8bbe\u5907:");
                        topPanel.add(label6);

                        //---- sender ----
                        sender.setText("text");
                        topPanel.add(sender);
                    }
                    panel1.add(topPanel);

                    //======== topPanel2 ========
                    {
                        topPanel2.setLayout(new FlowLayout(FlowLayout.LEFT, 10, 5));

                        //---- label7 ----
                        label7.setText("\u6d88\u606f\u89e3\u91ca:");
                        topPanel2.add(label7);

                        //---- messageComment ----
                        messageComment.setColumns(20);
                        messageComment.setEditable(false);
                        topPanel2.add(messageComment);

                        //---- label8 ----
                        label8.setText("\u7528\u6237\u5907\u6ce8:");
                        topPanel2.add(label8);

                        //---- userMessageComment ----
                        userMessageComment.setColumns(20);
                        userMessageComment.setEditable(false);
                        topPanel2.add(userMessageComment);
                    }
                    panel1.add(topPanel2);
                }
                contentPanel.add(panel1, BorderLayout.NORTH);

                //======== centerPanel ========
                {
                    centerPanel.setLayout(new BorderLayout());
                }
                contentPanel.add(centerPanel, BorderLayout.CENTER);
            }
            dialogPane.add(contentPanel, BorderLayout.CENTER);

            //======== panel2 ========
            {
                panel2.setLayout(new FlowLayout(FlowLayout.RIGHT, 10, 10));

                //---- showCommonCheckBox ----
                showCommonCheckBox.setText("\u663e\u793a\u89e3\u91ca");
                showCommonCheckBox.addChangeListener(e -> showCommonCheckBoxStateChanged(e));
                panel2.add(showCommonCheckBox);

                //---- saveBtn ----
                saveBtn.setText("save");
                panel2.add(saveBtn);

                //---- button2 ----
                button2.setText("ok");
                button2.addActionListener(e -> onOk(e));
                panel2.add(button2);

                //---- button3 ----
                button3.setText("cancel");
                button3.addActionListener(e -> onCancel(e));
                panel2.add(button3);
            }
            dialogPane.add(panel2, BorderLayout.SOUTH);
        }
        contentPane.add(dialogPane, BorderLayout.CENTER);
        setSize(985, 385);
        setLocationRelativeTo(getOwner());
        // JFormDesigner - End of component initialization  //GEN-END:initComponents  @formatter:on
    }

    // JFormDesigner - Variables declaration - DO NOT MODIFY  //GEN-BEGIN:variables  @formatter:off
    private JPanel dialogPane;
    private JPanel contentPanel;
    private JPanel panel1;
    private JPanel topPanel;
    private JLabel label1;
    private JLabel messageId;
    private JLabel label2;
    private JLabel messageName;
    private JLabel label3;
    private JLabel dataLength;
    private JLabel label4;
    private JLabel sendType;
    private JLabel label5;
    private JLabel cycleTime;
    private JLabel label6;
    private JLabel sender;
    private JPanel topPanel2;
    private JLabel label7;
    private JTextField messageComment;
    private JLabel label8;
    private JTextField userMessageComment;
    private JPanel centerPanel;
    private JPanel panel2;
    private JCheckBox showCommonCheckBox;
    private JButton saveBtn;
    private JButton button2;
    private JButton button3;
    // JFormDesigner - End of variables declaration  //GEN-END:variables  @formatter:on

    private CanMessage canMessage;
    private List<DbcSignal> dbcSignals;

    public static void main(String[] args) {
        MessageEditor view = new MessageEditor(null);
        view.setMaximumSize(new Dimension(800, 600));
        CanMessage message = new CanMessage();
        message.setProjectId(14L);
        message.setCanId("40");
        message.setData("7E86000F41001523");
        view.setMessage(message);
        view.refresh();
        view.setVisible(true);
    }
}
