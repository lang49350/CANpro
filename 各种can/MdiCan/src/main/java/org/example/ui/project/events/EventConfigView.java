/*
 * Created by JFormDesigner on Fri Aug 22 10:34:19 CST 2025
 */

package org.example.ui.project.events;

import java.awt.event.*;
import com.baomidou.mybatisplus.core.conditions.query.QueryWrapper;
import org.apache.commons.collections4.ListUtils;
import org.example.mapper.EventConfigMapper;
import org.example.mapper.EventRecordMapper;
import org.example.pojo.data.CanMessage;
import org.example.pojo.data.EventConfig;
import org.example.pojo.data.EventRecord;
import org.example.utils.db.DatabaseManager;
import org.example.utils.file.FileChooserUtils;
import org.example.utils.file.TxtFileReader;

import java.awt.*;
import java.io.File;
import java.util.ArrayList;
import java.util.List;
import javax.swing.*;
import javax.swing.border.*;

/**
 * @author jingq
 */
public class EventConfigView extends JDialog {
    private int currentId = 0;
    public EventConfigView(Window owner) {
        super(owner);
        initComponents();
        // 初始化数据
        init();
    }

    private void init() {
        DatabaseManager databaseManager = new DatabaseManager();
        List<EventConfig> eventConfigs = databaseManager.execute(EventConfig.class, session -> {
            return session.getMapper(EventConfigMapper.class).selectList(new QueryWrapper<>());
        });
        if(ListUtils.emptyIfNull(eventConfigs).isEmpty()) {
            return;
        }else{
            EventConfig eventConfig = eventConfigs.get(0);
            textFileName.setText(eventConfig.getName());
            textFieldCondition.setText(eventConfig.getEventCondition());
            textFieldWait1.setText(eventConfig.getWaitTime());
            textFieldWait2.setText(eventConfig.getWaitTime2());
            textFieldWait3.setText(eventConfig.getWaitTime3());
            textFieldDelta.setText(eventConfig.getDeltaTime());
            textFieldMessage1.setText(eventConfig.getMessageSendOnWait1());
            textFieldMessage2.setText(eventConfig.getMessageSendOnWait2());
            textFileLimit.setText(eventConfig.getLimitTimes().toString());
            if (eventConfig.getIsEnable() == 1) {
                checkBoxEnable.setSelected(true);
            } else {
                checkBoxEnable.setSelected(false);
            }
            currentId = eventConfig.getId().intValue();
            // 获取执行记录
            EventRecord record = databaseManager.execute(EventRecord.class, session -> {
                EventRecordMapper mapper = session.getMapper(EventRecordMapper.class);
               QueryWrapper<EventRecord> wrapper = new QueryWrapper<>();
               // 按照执行时间排序，倒序，最新的
               wrapper.lambda().eq(EventRecord::getName, eventConfig.getName())
                       .eq(EventRecord::getIsDel, 0)
                       .eq(EventRecord::getIsDel,0)
                       .orderByDesc(EventRecord::getRunningTime);
               List<EventRecord> eventRecords = mapper.selectList(wrapper);
               if(!ListUtils.emptyIfNull(eventRecords).isEmpty()) {
                   return eventRecords.get(0);
               }else{
                   return null;
               }
            });

            if(record != null){
                currentRunTime.setText(record.getRunningTimes().toString());
                currentRunWait.setText(
                        eventConfig.getWaitTime() + " + " +
                                eventConfig.getWaitTime2() + " + " +
                                record.getRunningTimes()  + " * " + Integer.parseInt(eventConfig.getDeltaTime())
                );
            }

        }
    }

    private void save(){
        EventConfig eventConfig = new EventConfig();
        eventConfig.setName(textFileName.getText());
        eventConfig.setEventCondition(textFieldCondition.getText());
        eventConfig.setWaitTime(textFieldWait1.getText());
        eventConfig.setWaitTime2(textFieldWait2.getText());
        eventConfig.setWaitTime3(textFieldWait3.getText());
        eventConfig.setDeltaTime(textFieldDelta.getText());
        eventConfig.setMessageSendOnWait1(textFieldMessage1.getText());
        eventConfig.setMessageSendOnWait2(textFieldMessage2.getText());
        eventConfig.setLimitTimes(Integer.parseInt(textFileLimit.getText()));
        if(checkBoxEnable.isSelected()){
            eventConfig.setIsEnable(1);
        }else{
            eventConfig.setIsEnable(0);
        }
        if(currentId > 0) {
            eventConfig.setId(Long.valueOf(currentId));
        }
        DatabaseManager databaseManager = new DatabaseManager();
        databaseManager.execute(EventConfig.class, session -> {
            EventConfigMapper eventConfigMapper = session.getMapper(EventConfigMapper.class);
            if(currentId > 0) {
                int updateById = eventConfigMapper.updateById(eventConfig);
                if(updateById > 0) {
                    JOptionPane.showMessageDialog(null, "更新成功");
                }
            }else{
                int insert = eventConfigMapper.insert(eventConfig);
                if(insert > 0) {
                    JOptionPane.showMessageDialog(null, "保存成功");
                }
            }
            return null;
        });
    }

    private void onOk(ActionEvent e) {
        // TODO add your code here
        save();
    }

    private void loadTxtForMsg1(ActionEvent e) {
        // TODO add your code here
        File file = FileChooserUtils.chooseTxtFile();
        String result = "";
        List<String> msgStrList = new ArrayList<>();
        if(file != null){
            List<String> strings = TxtFileReader.readFileToList(file);
            List<CanMessage> canMessageList = CanMessage.toCanMessageList(strings);
            for (CanMessage message : canMessageList){
                String canId = message.getCanId();
                String data = message.getData();
                msgStrList.add(canId + "_" + data);
            }
            textFieldMessage1.setText(String.join(",", msgStrList));
        }
    }

    private void loadFileForMsg2(ActionEvent e) {
        // TODO add your code here
        // TODO add your code here
        File file = FileChooserUtils.chooseTxtFile();
        String result = "";
        List<String> msgStrList = new ArrayList<>();
        if(file != null){
            List<String> strings = TxtFileReader.readFileToList(file);
            List<CanMessage> canMessageList = CanMessage.toCanMessageList(strings);
            for (CanMessage message : canMessageList){
                String canId = message.getCanId();
                String data = message.getData();
                msgStrList.add(canId + "_" + data);
            }
            textFieldMessage2.setText(String.join(",", msgStrList));
        }
    }

    private void initComponents() {
        // JFormDesigner - Component initialization - DO NOT MODIFY  //GEN-BEGIN:initComponents  @formatter:off
        dialogPane = new JPanel();
        contentPanel = new JPanel();
        panel = new JPanel();
        panel8 = new JPanel();
        label10 = new JLabel();
        currentRunTime = new JLabel();
        label14 = new JLabel();
        currentRunWait = new JLabel();
        panel1 = new JPanel();
        label2 = new JLabel();
        textFileName = new JTextField();
        label11 = new JLabel();
        textFileLimit = new JTextField();
        panel2 = new JPanel();
        label3 = new JLabel();
        textFieldCondition = new JTextField();
        panel3 = new JPanel();
        label4 = new JLabel();
        textFieldWait1 = new JTextField();
        panel4 = new JPanel();
        label5 = new JLabel();
        textFieldWait2 = new JTextField();
        panel9 = new JPanel();
        label12 = new JLabel();
        textFieldDelta = new JTextField();
        panel5 = new JPanel();
        label6 = new JLabel();
        textFieldMessage1 = new JTextField();
        button1 = new JButton();
        panel6 = new JPanel();
        label8 = new JLabel();
        textFieldWait3 = new JTextField();
        panel7 = new JPanel();
        label9 = new JLabel();
        textFieldMessage2 = new JTextField();
        button2 = new JButton();
        panel10 = new JPanel();
        checkBoxEnable = new JCheckBox();
        buttonBar = new JPanel();
        okButton = new JButton();
        cancelButton = new JButton();

        //======== this ========
        Container contentPane = getContentPane();
        contentPane.setLayout(new BorderLayout());

        //======== dialogPane ========
        {
            dialogPane.setBorder(new EmptyBorder(12, 12, 12, 12));
            dialogPane.setLayout(new BorderLayout());

            //======== contentPanel ========
            {
                contentPanel.setLayout(new BoxLayout(contentPanel, BoxLayout.X_AXIS));

                //======== panel ========
                {
                    panel.setLayout(new GridLayout(11, 0));

                    //======== panel8 ========
                    {
                        panel8.setLayout(new GridLayout());

                        //---- label10 ----
                        label10.setText("\u5f53\u524d\u6267\u884c\u6b21\u6570");
                        panel8.add(label10);

                        //---- currentRunTime ----
                        currentRunTime.setText("0");
                        panel8.add(currentRunTime);

                        //---- label14 ----
                        label14.setText("\u5f53\u524d\u5ef6\u65f6");
                        panel8.add(label14);

                        //---- currentRunWait ----
                        currentRunWait.setText("0");
                        panel8.add(currentRunWait);
                    }
                    panel.add(panel8);

                    //======== panel1 ========
                    {
                        panel1.setLayout(new GridLayout());

                        //---- label2 ----
                        label2.setText("\u4e8b\u4ef6\u540d\u79f0");
                        panel1.add(label2);
                        panel1.add(textFileName);

                        //---- label11 ----
                        label11.setText("\u9650\u5b9a\u6b21\u6570");
                        panel1.add(label11);
                        panel1.add(textFileLimit);
                    }
                    panel.add(panel1);

                    //======== panel2 ========
                    {
                        panel2.setLayout(new GridLayout());

                        //---- label3 ----
                        label3.setText("\u63cf\u8ff0");
                        panel2.add(label3);
                        panel2.add(textFieldCondition);
                    }
                    panel.add(panel2);

                    //======== panel3 ========
                    {
                        panel3.setLayout(new GridLayout());

                        //---- label4 ----
                        label4.setText("\u4e8b\u4ef6\u53d1\u9001\u540e\u7684\u7b49\u5f85\u65f6\u95f4(ms)");
                        panel3.add(label4);
                        panel3.add(textFieldWait1);
                    }
                    panel.add(panel3);

                    //======== panel4 ========
                    {
                        panel4.setLayout(new GridLayout());

                        //---- label5 ----
                        label5.setText("\u5ef6\u65f6\u65f6\u95f4\uff08ms\uff09");
                        panel4.add(label5);
                        panel4.add(textFieldWait2);
                    }
                    panel.add(panel4);

                    //======== panel9 ========
                    {
                        panel9.setLayout(new GridLayout());

                        //---- label12 ----
                        label12.setText(" delta time (ms)");
                        panel9.add(label12);
                        panel9.add(textFieldDelta);
                    }
                    panel.add(panel9);

                    //======== panel5 ========
                    {
                        panel5.setLayout(new BoxLayout(panel5, BoxLayout.X_AXIS));

                        //---- label6 ----
                        label6.setText("\u5ef6\u65f6\u5230\u540e\u53d1\u9001\u7684\u6d88\u606f,\u7ed3\u6784ID_DATA(ID\u4e0d\u5e260x\u524d\u7f00,16\u8fdb\u5236\uff0c\u591a\u4e2a\u6d88\u606f\u4f7f\u7528\u82f1\u6587\u9017\u53f7\u5206\u9694)");
                        panel5.add(label6);
                        panel5.add(textFieldMessage1);

                        //---- button1 ----
                        button1.setText("\u6587\u4ef6\u5bfc\u5165");
                        button1.setMaximumSize(new Dimension(85, 30));
                        button1.setPreferredSize(new Dimension(85, 30));
                        button1.setMinimumSize(new Dimension(85, 30));
                        button1.addActionListener(e -> loadTxtForMsg1(e));
                        panel5.add(button1);
                    }
                    panel.add(panel5);

                    //======== panel6 ========
                    {
                        panel6.setLayout(new GridLayout());

                        //---- label8 ----
                        label8.setText("\u7b49\u5f85\u65f6\u95f4\uff08ms\uff09");
                        panel6.add(label8);
                        panel6.add(textFieldWait3);
                    }
                    panel.add(panel6);

                    //======== panel7 ========
                    {
                        panel7.setLayout(new BoxLayout(panel7, BoxLayout.X_AXIS));

                        //---- label9 ----
                        label9.setText("\u7b49\u5f85\u65f6\u95f4\u5230\u540e\u53d1\u9001\u7684\u6d88\u606f");
                        panel7.add(label9);
                        panel7.add(textFieldMessage2);

                        //---- button2 ----
                        button2.setText("\u6587\u4ef6\u5bfc\u5165");
                        button2.addActionListener(e -> loadFileForMsg2(e));
                        panel7.add(button2);
                    }
                    panel.add(panel7);

                    //======== panel10 ========
                    {
                        panel10.setLayout(new GridLayout());

                        //---- checkBoxEnable ----
                        checkBoxEnable.setText("\u542f\u7528");
                        panel10.add(checkBoxEnable);
                    }
                    panel.add(panel10);
                }
                contentPanel.add(panel);
            }
            dialogPane.add(contentPanel, BorderLayout.CENTER);

            //======== buttonBar ========
            {
                buttonBar.setBorder(new EmptyBorder(12, 0, 0, 0));
                buttonBar.setLayout(new GridLayout());

                //---- okButton ----
                okButton.setText("\u4fdd\u5b58");
                okButton.addActionListener(e -> onOk(e));
                buttonBar.add(okButton);

                //---- cancelButton ----
                cancelButton.setText("Cancel");
                buttonBar.add(cancelButton);
            }
            dialogPane.add(buttonBar, BorderLayout.SOUTH);
        }
        contentPane.add(dialogPane, BorderLayout.CENTER);
        setSize(1060, 335);
        setLocationRelativeTo(getOwner());
        // JFormDesigner - End of component initialization  //GEN-END:initComponents  @formatter:on
    }

    // JFormDesigner - Variables declaration - DO NOT MODIFY  //GEN-BEGIN:variables  @formatter:off
    private JPanel dialogPane;
    private JPanel contentPanel;
    private JPanel panel;
    private JPanel panel8;
    private JLabel label10;
    private JLabel currentRunTime;
    private JLabel label14;
    private JLabel currentRunWait;
    private JPanel panel1;
    private JLabel label2;
    private JTextField textFileName;
    private JLabel label11;
    private JTextField textFileLimit;
    private JPanel panel2;
    private JLabel label3;
    private JTextField textFieldCondition;
    private JPanel panel3;
    private JLabel label4;
    private JTextField textFieldWait1;
    private JPanel panel4;
    private JLabel label5;
    private JTextField textFieldWait2;
    private JPanel panel9;
    private JLabel label12;
    private JTextField textFieldDelta;
    private JPanel panel5;
    private JLabel label6;
    private JTextField textFieldMessage1;
    private JButton button1;
    private JPanel panel6;
    private JLabel label8;
    private JTextField textFieldWait3;
    private JPanel panel7;
    private JLabel label9;
    private JTextField textFieldMessage2;
    private JButton button2;
    private JPanel panel10;
    private JCheckBox checkBoxEnable;
    private JPanel buttonBar;
    private JButton okButton;
    private JButton cancelButton;
    // JFormDesigner - End of variables declaration  //GEN-END:variables  @formatter:on

}
