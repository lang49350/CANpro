package org.example.ui.project;

import com.baomidou.mybatisplus.core.conditions.query.QueryWrapper;
import lombok.Getter;
import org.example.mapper.CProjectMapper;
import org.example.mapper.DbcMetadataMapper;
import org.example.pojo.dbc.DbcMetadata;
import org.example.pojo.project.CProject;
import org.example.utils.db.DatabaseManager;
import org.example.utils.table.EnhancedTable;

import javax.swing.*;
import javax.swing.table.DefaultTableModel;
import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.List;

public class OpenHistoryProjectDialog extends JDialog {
    private final EnhancedTable<CProject> enhancedTable;
    @Getter
    private CProject selectedFile;
    private boolean isFileSelected = false;

    public OpenHistoryProjectDialog(Frame parent, List<CProject> dbcFiles) {
        super(parent, "选择项目", true);
        enhancedTable = new EnhancedTable<>(CProject.class);
        setSize(600, 400);
        setLocationRelativeTo(parent);
        setLayout(new BorderLayout());


        // 创建表格
        JScrollPane scrollPane = new JScrollPane(enhancedTable);
        add(scrollPane, BorderLayout.CENTER);

        // 添加数据
        enhancedTable.appendBatch(dbcFiles);

        // 创建按钮面板
        JPanel buttonPanel = new JPanel();
        JButton selectButton = new JButton("选择");
        JButton cancelButton = new JButton("取消");
        JButton delButton = new JButton("删除");

        selectButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                int selectedRow = enhancedTable.getSelectedRow();
                if (selectedRow != -1) {
                    selectedFile  = enhancedTable.getDataAt(selectedRow);
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

        delButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                int selectedRow = enhancedTable.getSelectedRow();
                if (selectedRow != -1) {
                    selectedFile  = enhancedTable.getDataAt(selectedRow);
                    DatabaseManager databaseManager = new DatabaseManager();
                    databaseManager.execute(CProject.class,session -> {
                        CProjectMapper mapper = session.getMapper(CProjectMapper.class);
                        mapper.deleteById(selectedFile.getId());
                        enhancedTable.removeAll();
                        QueryWrapper<CProject> wrapper = new QueryWrapper<>();
                        List<CProject> cProjects = mapper.selectList(wrapper);
                        enhancedTable.appendBatch(cProjects);
                        return null;
                    });
                }
            }
        });

        buttonPanel.add(selectButton);
        buttonPanel.add(cancelButton);
        buttonPanel.add(delButton);
        add(buttonPanel, BorderLayout.SOUTH);
    }



    public boolean isFileSelected() {
        return isFileSelected;
    }

    public static void main(String[] args) {
        SwingUtilities.invokeLater(new Runnable() {
            @Override
            public void run() {
                DatabaseManager databaseManager = new DatabaseManager();
                List<CProject> projects = databaseManager.execute(CProject.class, session->{
                    CProjectMapper mapper = session.getMapper(CProjectMapper.class);
                    return mapper.selectList(null);
                });
                OpenHistoryProjectDialog dialog = new OpenHistoryProjectDialog(null, projects);
                dialog.setVisible(true);
            }
        });
    }
}
