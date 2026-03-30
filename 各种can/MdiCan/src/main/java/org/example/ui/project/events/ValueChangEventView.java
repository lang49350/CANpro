/*
 * Created by JFormDesigner on Wed Sep 10 08:42:52 CST 2025
 */

package org.example.ui.project.events;

import java.awt.event.*;

import org.example.mapper.ValueChangConfigMapper;
import org.example.pojo.data.CanMessage;
import org.example.pojo.data.ValueChangConfig;
import org.example.utils.db.DatabaseManager;
import org.example.utils.db.EntityTableGenerator;
import org.example.utils.db.InitAllTable;
import org.example.utils.event.EventListener;
import org.example.utils.event.EventManager;
import org.example.utils.event.EventString;
import org.example.utils.table.EnhancedTable;

import java.awt.*;
import java.util.Collections;
import javax.swing.*;
import javax.swing.border.*;
import java.util.List;
/**
 * @author jingq
 */
public class ValueChangEventView extends JFrame {
    private EnhancedTable<ValueChangConfig> table;
    private JScrollPane scrollPane;
    public ValueChangEventView(Window owner) {
        super();
        setTitle("ValueChangeEvent");
        initComponents();
        init();
    }

    private void init() {
        // 最大化窗口
        setMaximumSize(getToolkit().getScreenSize());
        table = new EnhancedTable<>(ValueChangConfig.class);
        table.setUseVerboserName(true);
        table.setUnShowColumns(Collections.singletonList("id"));
        table.initColumns();
        scrollPane = new JScrollPane(table);
        contentPanel.add(scrollPane, BorderLayout.CENTER);

        loadData();

    }

    private void loadData() {
        table.removeAll();
        DatabaseManager databaseManager = new DatabaseManager();
        databaseManager.execute(ValueChangConfig.class, (session) -> {
            ValueChangConfigMapper mapper = session.getMapper(ValueChangConfigMapper.class);
            List<ValueChangConfig> list = mapper.selectList(null);
            table.appendBatch(list);
            return null;
        });
    }

    private void addNew(ActionEvent e) {
        // TODO add your code here
        ValueChangConfig config = new ValueChangConfig();
        table.append(config);
    }

    private void save(ActionEvent e) {
        // TODO add your code here
        int selectedRow = table.getSelectedRow();
        if(selectedRow == -1) {
            JOptionPane.showMessageDialog(this, "请选择要保存的行");
            return;
        }
        ValueChangConfig dataAt = table.getDataAt(selectedRow);
        DatabaseManager databaseManager = new DatabaseManager();
        databaseManager.execute(ValueChangConfig.class, (session) -> {
            ValueChangConfigMapper mapper = session.getMapper(ValueChangConfigMapper.class);
            if(dataAt.getId() == null) {
               return mapper.insert(dataAt);
            } else {
               return mapper.updateById(dataAt);
            }
        });
        loadData();
        EventManager.getInstance().notifyListeners(EventString.UPDATE_VALUE_CHANGE_CONFIG.name());
        JOptionPane.showMessageDialog(this, "save success");
    }

    private void del(ActionEvent e) {
        // TODO add your code here
        int selectedRow = table.getSelectedRow();
        if(selectedRow == -1) {
            JOptionPane.showMessageDialog(this, "请选择要删除的行");
            return;
        }
        ValueChangConfig dataAt = table.getDataAt(selectedRow);
        DatabaseManager databaseManager = new DatabaseManager();
        databaseManager.execute(ValueChangConfig.class, (session) -> {
            ValueChangConfigMapper mapper = session.getMapper(ValueChangConfigMapper.class);
            if(dataAt.getId() == null) {
               return null;
            } else {
               return mapper.deleteById(dataAt.getId());
            }
        });
        loadData();
    }

    private void button3(ActionEvent e) {
        // TODO add your code here
        TxtFileParseView txtFileParseView = new TxtFileParseView(this);
        txtFileParseView.setVisible(true);
    }

    private void initComponents() {
        // JFormDesigner - Component initialization - DO NOT MODIFY  //GEN-BEGIN:initComponents  @formatter:off
        dialogPane = new JPanel();
        contentPanel = new JPanel();
        buttonBar = new JPanel();
        button2 = new JButton();
        okButton = new JButton();
        button1 = new JButton();
        cancelButton = new JButton();
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
                contentPanel.setLayout(new GridLayout());
            }
            dialogPane.add(contentPanel, BorderLayout.CENTER);

            //======== buttonBar ========
            {
                buttonBar.setBorder(new EmptyBorder(12, 0, 0, 0));
                buttonBar.setLayout(new FlowLayout());

                //---- button2 ----
                button2.setText("+");
                button2.addActionListener(e -> addNew(e));
                buttonBar.add(button2);

                //---- okButton ----
                okButton.setText("save");
                okButton.addActionListener(e -> save(e));
                buttonBar.add(okButton);

                //---- button1 ----
                button1.setText("del");
                button1.addActionListener(e -> del(e));
                buttonBar.add(button1);

                //---- cancelButton ----
                cancelButton.setText("Cancel");
                buttonBar.add(cancelButton);

                //---- button3 ----
                button3.setText("\u6587\u4ef6\u52a9\u624b");
                button3.addActionListener(e -> button3(e));
                buttonBar.add(button3);
            }
            dialogPane.add(buttonBar, BorderLayout.SOUTH);
        }
        contentPane.add(dialogPane, BorderLayout.CENTER);
        setSize(1160, 480);
        setLocationRelativeTo(getOwner());
        // JFormDesigner - End of component initialization  //GEN-END:initComponents  @formatter:on
    }

    // JFormDesigner - Variables declaration - DO NOT MODIFY  //GEN-BEGIN:variables  @formatter:off
    private JPanel dialogPane;
    private JPanel contentPanel;
    private JPanel buttonBar;
    private JButton button2;
    private JButton okButton;
    private JButton button1;
    private JButton cancelButton;
    private JButton button3;
    // JFormDesigner - End of variables declaration  //GEN-END:variables  @formatter:on

    public static void main(String[] args) {
        ValueChangEventView view = new ValueChangEventView(null);
        view.setVisible(true);
        view.setDefaultCloseOperation(JDialog.DISPOSE_ON_CLOSE);
    }
}
