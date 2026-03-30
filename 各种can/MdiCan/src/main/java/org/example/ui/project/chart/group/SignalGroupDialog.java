package org.example.ui.project.chart.group;

import org.example.pojo.project.SignalGroup;

import javax.swing.*;
import java.awt.*;
import java.text.SimpleDateFormat;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import java.util.Date;
import java.util.logging.SimpleFormatter;

public class SignalGroupDialog extends JDialog {
    private JTextField nameField;
    private JTextField creatorField;
    private JTextArea remarkArea;
    private JButton okButton;
    private JButton cancelButton;
    private boolean okClicked = false;
    private SignalGroup group;

    public SignalGroupDialog(Frame parent, String title, boolean modal, SignalGroup group) {
        super(parent, title, modal);
        this.group = group;
        initComponents();
        if (group != null) {
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

        JLabel nameLabel = new JLabel("名称:");
        gbc.gridx = 0;
        gbc.gridy = 0;
        panel.add(nameLabel, gbc);

        nameField = new JTextField(20);
        gbc.gridx = 1;
        gbc.gridy = 0;
        panel.add(nameField, gbc);

        JLabel creatorLabel = new JLabel("创建人:");
        gbc.gridx = 0;
        gbc.gridy = 1;
        panel.add(creatorLabel, gbc);

        creatorField = new JTextField(20);
        gbc.gridx = 1;
        gbc.gridy = 1;
        panel.add(creatorField, gbc);

        JLabel remarkLabel = new JLabel("备注:");
        gbc.gridx = 0;
        gbc.gridy = 2;
        panel.add(remarkLabel, gbc);

        remarkArea = new JTextArea(5, 20);
        JScrollPane scrollPane = new JScrollPane(remarkArea);
        gbc.gridx = 1;
        gbc.gridy = 2;
        panel.add(scrollPane, gbc);

        JPanel buttonPanel = new JPanel();
        okButton = new JButton("确定");
        cancelButton = new JButton("取消");
        buttonPanel.add(okButton);
        buttonPanel.add(cancelButton);

        getContentPane().add(panel, BorderLayout.CENTER);
        getContentPane().add(buttonPanel, BorderLayout.SOUTH);

        okButton.addActionListener(e -> {
            if (validateInput()) {
                if (group == null) {
                    SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
                    String now = sdf.format(new Date());
                    group = new SignalGroup(null,nameField.getText(), creatorField.getText(),now,remarkArea.getText());
                } else {
                    group.setName(nameField.getText());
                    group.setCreator(creatorField.getText());
                    group.setRemark(remarkArea.getText());
                }
                okClicked = true;
                dispose();
            }
        });

        cancelButton.addActionListener(e -> dispose());
    }

    private boolean validateInput() {
        if (nameField.getText().trim().isEmpty()) {
            JOptionPane.showMessageDialog(this, "请输入信号组名称", "输入错误", JOptionPane.ERROR_MESSAGE);
            return false;
        }
        if (creatorField.getText().trim().isEmpty()) {
            JOptionPane.showMessageDialog(this, "请输入创建人", "输入错误", JOptionPane.ERROR_MESSAGE);
            return false;
        }
        return true;
    }

    private void fillFields() {
        nameField.setText(group.getName());
        creatorField.setText(group.getCreator());
        remarkArea.setText(group.getRemark());
    }

    public boolean isOkClicked() {
        return okClicked;
    }

    public SignalGroup getGroup() {
        return group;
    }

    public static void main(String[] args) {
        SignalGroup group = new SignalGroup(1, "测试信号组", "admin",
                new SimpleDateFormat("yyyy-MM-dd HH:mm:ss").format(LocalDateTime.now()), "测试信号组");
        SignalGroupDialog dialog = new SignalGroupDialog(null, "添加信号组", true, group);
        dialog.setVisible(true);
        if (dialog.isOkClicked()) {
            System.out.println("名称: " + dialog.getGroup().getName());
            System.out.println("创建人: " + dialog.getGroup().getCreator());
        }
    }
}    