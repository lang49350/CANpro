package org.example.ui.project.chart.group;

import lombok.Data;
import lombok.EqualsAndHashCode;
import org.apache.commons.lang3.StringUtils;
import org.apache.commons.math3.analysis.function.Sin;
import org.example.mapper.DbcMessageMapper;
import org.example.pojo.dbc.DbcMessage;
import org.example.pojo.project.SignalGroup;
import org.example.pojo.project.SignalInGroup;
import org.example.utils.db.DatabaseManager;

import javax.swing.*;
import java.awt.*;
import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

@EqualsAndHashCode(callSuper = true)
@Data
public class SignalDialog extends JDialog {
    private JTextField nameField;
    private JTextField messageIdField;
    private JTextField messageNameField;
    private JTextField senderField;
    private JTextField cycleField;
    private JTextField maxValueField;
    private JTextField minValueField;
    private JTextArea remarkArea;
    private JComboBox<String> groupComboBox;
    private JButton okButton;
    private JButton cancelButton;
    private boolean okClicked = false;
    private SignalInGroup mysignal;
    private List<SignalGroup> groups;

    // 新增字段
    private JTextField start_bitField;
    private JTextField lengthField;
    private JTextField byteOrderField;
    private JCheckBox isSignedCheckBox;
    private JTextField scaleField;
    private JTextField offsetField;

    public SignalDialog(Frame parent, String title, boolean modal, SignalInGroup signal, List<SignalGroup> groups) {
        super(parent, title, modal);
        this.groups = groups;
        this.mysignal = signal == null ? new SignalInGroup() : signal;
        initComponents();
        if (signal != null) {
            fillFields();
        }
        pack();
        setLocationRelativeTo(parent);
        setResizable(false);
    }

    private void initComponents() {
        JPanel panel = new JPanel(new GridBagLayout());
        GridBagConstraints gbc = new GridBagConstraints();
        gbc.insets = new Insets(5, 5, 5, 5);
        gbc.fill = GridBagConstraints.HORIZONTAL;

        // 基本信息字段
        JLabel nameLabel = new JLabel("名称:");
        gbc.gridx = 0;
        gbc.gridy = 0;
        panel.add(nameLabel, gbc);

        nameField = new JTextField(20);
        gbc.gridx = 1;
        gbc.gridy = 0;
        panel.add(nameField, gbc);

        JLabel messageIdLabel = new JLabel("消息ID:");
        gbc.gridx = 0;
        gbc.gridy = 1;
        panel.add(messageIdLabel, gbc);

        messageIdField = new JTextField(20);
        gbc.gridx = 1;
        gbc.gridy = 1;
        panel.add(messageIdField, gbc);

        JLabel messageNameLabel = new JLabel("消息名称:");
        gbc.gridx = 0;
        gbc.gridy = 2;
        panel.add(messageNameLabel, gbc);

        messageNameField = new JTextField(20);
        gbc.gridx = 1;
        gbc.gridy = 2;
        panel.add(messageNameField, gbc);

        JLabel senderLabel = new JLabel("发送节点:");
        gbc.gridx = 0;
        gbc.gridy = 3;
        panel.add(senderLabel, gbc);

        senderField = new JTextField(20);
        gbc.gridx = 1;
        gbc.gridy = 3;
        panel.add(senderField, gbc);

        JLabel cycleLabel = new JLabel("周期(ms):");
        gbc.gridx = 0;
        gbc.gridy = 4;
        panel.add(cycleLabel, gbc);

        cycleField = new JTextField(20);
        gbc.gridx = 1;
        gbc.gridy = 4;
        panel.add(cycleField, gbc);

        JLabel maxValueLabel = new JLabel("最大值:");
        gbc.gridx = 0;
        gbc.gridy = 5;
        panel.add(maxValueLabel, gbc);

        maxValueField = new JTextField(20);
        gbc.gridx = 1;
        gbc.gridy = 5;
        panel.add(maxValueField, gbc);

        JLabel minValueLabel = new JLabel("最小值:");
        gbc.gridx = 0;
        gbc.gridy = 6;
        panel.add(minValueLabel, gbc);

        minValueField = new JTextField(20);
        gbc.gridx = 1;
        gbc.gridy = 6;
        panel.add(minValueField, gbc);

        JLabel groupLabel = new JLabel("所属组:");
        gbc.gridx = 0;
        gbc.gridy = 7;
        panel.add(groupLabel, gbc);

        String[] groupNames = new String[groups.size()];
        for (int i = 0; i < groups.size(); i++) {
            groupNames[i] = groups.get(i).getName();
        }
        groupComboBox = new JComboBox<>(groupNames);
        gbc.gridx = 1;
        gbc.gridy = 7;
        panel.add(groupComboBox, gbc);

        // 新增字段
        JLabel start_bitLabel = new JLabel("起始位:");
        gbc.gridx = 0;
        gbc.gridy = 8;
        panel.add(start_bitLabel, gbc);

        start_bitField = new JTextField(20);
        gbc.gridx = 1;
        gbc.gridy = 8;
        panel.add(start_bitField, gbc);

        JLabel lengthLabel = new JLabel("长度:");
        gbc.gridx = 0;
        gbc.gridy = 9;
        panel.add(lengthLabel, gbc);

        lengthField = new JTextField(20);
        gbc.gridx = 1;
        gbc.gridy = 9;
        panel.add(lengthField, gbc);

        JLabel byteOrderLabel = new JLabel("字节序:");
        gbc.gridx = 0;
        gbc.gridy = 10;
        panel.add(byteOrderLabel, gbc);

        byteOrderField = new JTextField(20);
        gbc.gridx = 1;
        gbc.gridy = 10;
        byteOrderField.setText("little_endian");
        panel.add(byteOrderField, gbc);

        JLabel isSignedLabel = new JLabel("是否有符号:");
        gbc.gridx = 0;
        gbc.gridy = 11;
        panel.add(isSignedLabel, gbc);

        isSignedCheckBox = new JCheckBox();
        gbc.gridx = 1;
        gbc.gridy = 11;
        panel.add(isSignedCheckBox, gbc);

        JLabel scaleLabel = new JLabel("缩放因子:");
        gbc.gridx = 0;
        gbc.gridy = 12;
        panel.add(scaleLabel, gbc);

        scaleField = new JTextField(20);
        gbc.gridx = 1;
        gbc.gridy = 12;
        panel.add(scaleField, gbc);

        JLabel offsetLabel = new JLabel("偏移量:");
        gbc.gridx = 0;
        gbc.gridy = 13;
        panel.add(offsetLabel, gbc);

        offsetField = new JTextField(20);
        gbc.gridx = 1;
        gbc.gridy = 13;
        panel.add(offsetField, gbc);

        // 备注字段移至最底部
        JLabel remarkLabel = new JLabel("备注:");
        gbc.gridx = 0;
        gbc.gridy = 14;
        panel.add(remarkLabel, gbc);

        remarkArea = new JTextArea(5, 20);
        JScrollPane scrollPane = new JScrollPane(remarkArea);
        gbc.gridx = 1;
        gbc.gridy = 14;
        panel.add(scrollPane, gbc);

        // 按钮面板
        JPanel buttonPanel = new JPanel();
        okButton = new JButton("确定");
        cancelButton = new JButton("取消");
        buttonPanel.add(okButton);
        buttonPanel.add(cancelButton);

        getContentPane().add(panel, BorderLayout.CENTER);
        getContentPane().add(buttonPanel, BorderLayout.SOUTH);


        okButton.addActionListener(e -> {
            if (validateInput()) {
                int groupId = groups.get(groupComboBox.getSelectedIndex()).getId();
                mysignal.setName(nameField.getText());
                mysignal.setMessageId(messageIdField.getText());
                mysignal.setMaximum(maxValueField.getText());
                mysignal.setMinimum(minValueField.getText());
                mysignal.setUserComment(remarkArea.getText());
                mysignal.setGroupId(groupId);
                mysignal.setStartBit(start_bitField.getText());
                mysignal.setLength(lengthField.getText());
                mysignal.setByteOrder(byteOrderField.getText());
                mysignal.setIsSigned(isSignedCheckBox.isSelected() ?"1":"0");
                mysignal.setScale(scaleField.getText());
                mysignal.setOffset(offsetField.getText());
                okClicked = true;
                dispose();
            }
        });

        cancelButton.addActionListener(e -> dispose());
    }

    private boolean validateInput() {
        if (nameField.getText().trim().isEmpty()) {
            JOptionPane.showMessageDialog(this, "请输入信号名称", "输入错误", JOptionPane.ERROR_MESSAGE);
            return false;
        }
        if (messageIdField.getText().trim().isEmpty()) {
            JOptionPane.showMessageDialog(this, "请输入消息ID", "输入错误", JOptionPane.ERROR_MESSAGE);
            return false;
        }
        if (messageNameField.getText().trim().isEmpty()) {
            JOptionPane.showMessageDialog(this, "请输入消息名称", "输入错误", JOptionPane.ERROR_MESSAGE);
            return false;
        }
        if (senderField.getText().trim().isEmpty()) {
            JOptionPane.showMessageDialog(this, "请输入发送节点", "输入错误", JOptionPane.ERROR_MESSAGE);
            return false;
        }
        try {
            Integer.parseInt(cycleField.getText());
        } catch (NumberFormatException e) {
            JOptionPane.showMessageDialog(this, "周期必须是整数", "输入错误", JOptionPane.ERROR_MESSAGE);
            return false;
        }
        try {
            Double.parseDouble(maxValueField.getText());
            Double.parseDouble(minValueField.getText());
        } catch (NumberFormatException e) {
            JOptionPane.showMessageDialog(this, "最大值和最小值必须是数字", "输入错误", JOptionPane.ERROR_MESSAGE);
            return false;
        }
        try {
            Integer.parseInt(start_bitField.getText());
            Integer.parseInt(lengthField.getText());
            Double.parseDouble(scaleField.getText());
            Double.parseDouble(offsetField.getText());
        } catch (NumberFormatException e) {
            JOptionPane.showMessageDialog(this, "起始位、长度、缩放因子和偏移量必须是数字", "输入错误", JOptionPane.ERROR_MESSAGE);
            return false;
        }
        return true;
    }

    private void fillFields() {
        nameField.setText(mysignal.getName());
        messageIdField.setText(mysignal.getMessageId());
        String messageId = mysignal.getMessageId();
        DatabaseManager dbManager = new DatabaseManager();
        DbcMessage message = dbManager.execute(DbcMessage.class, session -> {
            return session.getMapper(DbcMessageMapper.class).selectById(messageId);
        });
        if (message != null){
            messageNameField.setText(StringUtils.defaultIfBlank(message.getName(),""));
            senderField.setText(StringUtils.defaultIfBlank(message.getSender(),""));
            cycleField.setText(StringUtils.defaultIfBlank(message.getCycleTime(),""));
            maxValueField.setText(StringUtils.defaultIfBlank(mysignal.getMaximum(),""));
            minValueField.setText(StringUtils.defaultIfBlank(mysignal.getMinimum(),""));
            remarkArea.setText(StringUtils.defaultIfBlank(mysignal.getUserComment(),""));

            // 设置所属组
            for (int i = 0; i < groups.size(); i++) {
                if (groups.get(i).getId() == mysignal.getGroupId()) {
                    groupComboBox.setSelectedIndex(i);
                    break;
                }
            }

            // 填充新增字段
            start_bitField.setText(StringUtils.defaultIfBlank(mysignal.getStartBit(),"0"));
            lengthField.setText(StringUtils.defaultIfBlank(String.valueOf(mysignal.getLength()),""));
            byteOrderField.setText(StringUtils.defaultIfBlank(mysignal.getByteOrder(),"little_endian"));
            isSignedCheckBox.setSelected(Objects.equals(mysignal.getIsSigned(), "1"));
            scaleField.setText(StringUtils.defaultIfBlank(mysignal.getScale(),"1"));
            offsetField.setText(StringUtils.defaultIfBlank(mysignal.getOffset(),"0"));
        }

    }



    public static void main(String[] args) {
        SignalInGroup signalInGroup = new SignalInGroup();
        signalInGroup.setName("Speed");
        signalInGroup.setMessageId("0x201");
        signalInGroup.setMaximum("100");
        signalInGroup.setMinimum("0");
        signalInGroup.setUserComment("车速");
        signalInGroup.setGroupId(1);
        signalInGroup.setStartBit("0");
        signalInGroup.setLength("16");
        signalInGroup.setByteOrder("little_endian");
        signalInGroup.setIsSigned("1");
        signalInGroup.setScale("1");
        signalInGroup.setOffset("0");

        System.out.println(signalInGroup);


    }
}