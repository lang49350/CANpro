package org.example.ui.project;

import lombok.Getter;
import org.example.mapper.DbcMetadataMapper;
import org.example.pojo.data.CanMessage;
import org.example.pojo.dbc.DbcMetadata;
import org.example.utils.db.DatabaseManager;

import javax.swing.*;
import javax.swing.table.DefaultTableModel;
import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.List;

public class DBCFileSelectionDialog extends JDialog {
    private JTable table;
    private JButton selectButton;
    private JButton cancelButton;
    @Getter
    private DbcMetadata selectedFile;
    private boolean isFileSelected = false;

    public DBCFileSelectionDialog(Frame parent, List<DbcMetadata> dbcFiles) {
        super(parent, "选择DBC文件", true);
        setSize(600, 400);
        setLocationRelativeTo(parent);
        setLayout(new BorderLayout());

        // 创建表格模型
        DefaultTableModel model = new DefaultTableModel(
                new String[]{"ID", "文件名称", "备注信息"}, 0) {
            @Override
            public boolean isCellEditable(int row, int column) {
                return false;
            }
        };

        // 添加数据到表格
        for (DbcMetadata file : dbcFiles) {
            model.addRow(new Object[]{file.getId(), file.getFilename(), file.getComment()});
        }

        // 创建表格
        table = new JTable(model);
        JScrollPane scrollPane = new JScrollPane(table);
        add(scrollPane, BorderLayout.CENTER);

        // 创建按钮面板
        JPanel buttonPanel = new JPanel();
        selectButton = new JButton("选择");
        cancelButton = new JButton("取消");

        selectButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                int selectedRow = table.getSelectedRow();
                if (selectedRow != -1) {
                    Long id = (Long) table.getValueAt(selectedRow, 0);
                    DatabaseManager databaseManager = new DatabaseManager();
                    selectedFile  = databaseManager.execute(DbcMetadata.class, session -> {
                        DbcMetadataMapper mapper = session.getMapper(DbcMetadataMapper.class);
                        return mapper.selectById(id);
                    });
                    isFileSelected = true;
                }
                dispose();
            }
        });

        cancelButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                dispose();
            }
        });

        buttonPanel.add(selectButton);
        buttonPanel.add(cancelButton);
        add(buttonPanel, BorderLayout.SOUTH);
    }


    public boolean isFileSelected() {
        return isFileSelected;
    }

}