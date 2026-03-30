package org.example.ui.project;

import lombok.Data;
import lombok.Getter;
import org.example.mapper.CProjectMapper;
import org.example.mapper.DbcMetadataMapper;
import org.example.pojo.dbc.DbcMetadata;
import org.example.pojo.project.CProject;
import org.example.utils.db.DatabaseManager;
import org.example.utils.web.UdpWifiDataReceiver;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.swing.*;
import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.List;

public class ProjectFormDialog extends JDialog {
    private static final Logger logger = LoggerFactory.getLogger(ProjectFormDialog.class);
    private JTextField nameField;
    private JTextField createTimeField;
    private JTextField createByField;
    private JTextField dbcIdField;
    private JTextArea descriptionArea;
    private JButton selectDBCButton;
    private JButton submitButton;
    private JButton cancelButton;
    private CProject project;
    private List<DbcMetadata> dbcFiles;

    private boolean isSelected = false;

    private Long projectId;

    public boolean getIsSelected() {
        return isSelected;
    }

    public CProject getProject(){
        return project;
    }

    public ProjectFormDialog(Frame parent) {
        super(parent, "项目信息表单", true);
        this.dbcFiles = dbcFiles;
        setSize(600, 500);
        setLocationRelativeTo(parent);
        setLayout(new BorderLayout());

        // 创建表单面板
        JPanel formPanel = new JPanel(new GridBagLayout());
        GridBagConstraints gbc = new GridBagConstraints();
        gbc.insets = new Insets(5, 5, 5, 5);
        gbc.anchor = GridBagConstraints.WEST;

        // 项目名称
        gbc.gridx = 0;
        gbc.gridy = 0;
        formPanel.add(new JLabel("项目名称:"), gbc);

        gbc.gridx = 1;
        nameField = new JTextField(20);
        formPanel.add(nameField, gbc);

        // 创建时间
        gbc.gridx = 0;
        gbc.gridy = 1;
        formPanel.add(new JLabel("创建时间 (yyyy-MM-dd):"), gbc);

        gbc.gridx = 1;
        createTimeField = new JTextField(20);
        createTimeField.setText(new SimpleDateFormat("yyyy-MM-dd").format(new Date()));
        formPanel.add(createTimeField, gbc);

        // 创建人
        gbc.gridx = 0;
        gbc.gridy = 2;
        formPanel.add(new JLabel("创建人:"), gbc);

        gbc.gridx = 1;
        createByField = new JTextField(20);
        formPanel.add(createByField, gbc);

        // DBC文件ID
        gbc.gridx = 0;
        gbc.gridy = 3;
        formPanel.add(new JLabel("DBC文件ID:"), gbc);

        gbc.gridx = 1;
        dbcIdField = new JTextField(20);
        dbcIdField.setEditable(false);
        formPanel.add(dbcIdField, gbc);

        // 选择DBC文件按钮
        gbc.gridx = 2;
        selectDBCButton = new JButton("选择DBC文件");
        formPanel.add(selectDBCButton, gbc);

        // 描述信息
        gbc.gridx = 0;
        gbc.gridy = 4;
        formPanel.add(new JLabel("描述信息:"), gbc);

        gbc.gridx = 1;
        gbc.gridwidth = 2;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        descriptionArea = new JTextArea(5, 20);
        JScrollPane scrollPane = new JScrollPane(descriptionArea);
        formPanel.add(scrollPane, gbc);

        // 添加表单面板到对话框
        add(formPanel, BorderLayout.CENTER);

        // 创建按钮面板
        JPanel buttonPanel = new JPanel();
        submitButton = new JButton("提交");
        cancelButton = new JButton("取消");

        submitButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                if (validateForm()) {
                    project = new CProject();
                    project.setName(nameField.getText());

                    project.setCreateTime(createTimeField.getText());

                    project.setCreateBy(createByField.getText());

                    if (!dbcIdField.getText().isEmpty()) {
                        project.setDbcMetadataId(Long.parseLong(dbcIdField.getText()));
                    }

                    project.setDescription(descriptionArea.getText());
                    // 存库
                    DatabaseManager dbManager = new DatabaseManager();
                    Integer execute = dbManager.execute(CProject.class, session -> {
                        CProjectMapper mapper = session.getMapper(CProjectMapper.class);
                        if(projectId != null){
                            project.setId(projectId);
                        }
                        if (project.getId() != null){
                            return mapper.updateById(project);
                        }else{
                            return mapper.insert(project);
                        }
                    });
                    isSelected = true;
                    dispose();
                }
            }
        });

        cancelButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                dispose();
            }
        });

        selectDBCButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                DBCFileSelectionDialog dialog = new DBCFileSelectionDialog(
                        null, dbcFiles);
                dialog.setVisible(true);

                if (dialog.isFileSelected()) {
                    DbcMetadata selectedFile = dialog.getSelectedFile();
                    dbcIdField.setText(selectedFile.getId().toString());
                }
            }
        });

        // 获取数据
        DatabaseManager dbManager = new DatabaseManager();
        dbcFiles  = dbManager.execute(DbcMetadata.class,session ->
                session.getMapper(DbcMetadataMapper.class).selectList(null));

        buttonPanel.add(submitButton);
        buttonPanel.add(cancelButton);
        add(buttonPanel, BorderLayout.SOUTH);
    }

    private boolean validateForm() {
        if (nameField.getText().isEmpty()) {
            JOptionPane.showMessageDialog(this, "项目名称不能为空", "错误", JOptionPane.ERROR_MESSAGE);
            return false;
        }

        if (createTimeField.getText().isEmpty()) {
            JOptionPane.showMessageDialog(this, "创建时间不能为空", "错误", JOptionPane.ERROR_MESSAGE);
            return false;
        }

        if (createByField.getText().isEmpty()) {
            JOptionPane.showMessageDialog(this, "创建人不能为空", "错误", JOptionPane.ERROR_MESSAGE);
            return false;
        }

        return true;
    }

    public void setProject(CProject cProject) {
        this.projectId = cProject.getId();
        this.nameField.setText(cProject.getName());
        if(cProject.getCreateBy() !=null){
            this.createByField.setText(cProject.getCreateBy());
        }

        this.createTimeField.setText(cProject.getCreateTime());
        if(cProject.getDescription() != null){
            this.descriptionArea.setText(cProject.getDescription());
        }

        if(cProject.getDbcMetadataId() != null){
            this.dbcIdField.setText(cProject.getDbcMetadataId().toString());
        }

    }
}