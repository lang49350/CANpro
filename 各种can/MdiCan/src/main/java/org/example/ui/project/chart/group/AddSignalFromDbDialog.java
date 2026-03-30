/*
 * Created by JFormDesigner on Thu Jul 03 10:31:41 CST 2025
 */

package org.example.ui.project.chart.group;

import com.baomidou.mybatisplus.core.conditions.query.QueryWrapper;
import org.apache.commons.lang3.StringUtils;
import org.example.mapper.DbcMessageMapper;
import org.example.mapper.DbcMetadataMapper;
import org.example.mapper.DbcSignalMapper;
import org.example.pojo.dbc.DbcMessage;
import org.example.pojo.dbc.DbcMetadata;
import org.example.pojo.dbc.DbcSignal;
import org.example.pojo.project.SignalInGroup;
import org.example.utils.db.DatabaseManager;
import org.example.utils.table.EnhancedTable;
import org.springframework.beans.BeanUtils;

import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.WindowAdapter;
import java.util.ArrayList;
import java.util.List;
import java.util.Objects;
import javax.swing.*;
import javax.swing.border.*;
import javax.swing.event.ListSelectionEvent;
import javax.swing.event.ListSelectionListener;

import static javax.swing.JPopupMenu.setDefaultLightWeightPopupEnabled;

/**
 * @author jingq
 */
public class AddSignalFromDbDialog extends JFrame {

    private DbcMetadata currentDbcMetaData; // 当前DBC文件的指纹
    // 回调函数设置
    private SignalSelectionListener selectionListener;

    private Boolean isAdd = false;

    public void setSignalSelectionListener(SignalSelectionListener listener) {
        this.selectionListener = listener;
    }
    public AddSignalFromDbDialog(Window owner) {
        super();
        JPopupMenu.setDefaultLightWeightPopupEnabled(false);
        initComponents();
        // 最大化;
        setExtendedState(JFrame.MAXIMIZED_BOTH);
        messageTable = new EnhancedTable<>(DbcMessage.class);
        signalTable = new EnhancedTable<>(DbcSignal.class);
        signalInGroupTable = new EnhancedTable<>(SignalInGroup.class);
        // 初始化表格
        initTable();
        // 加载DBC文件清单
        comboBox1.addItem("");
        DatabaseManager dataBaseManager = new DatabaseManager();
        dataBaseManager.execute(DbcMetadata.class, (session) -> {
            session.getMapper(DbcMetadataMapper.class).selectList(null).forEach(dbc -> {
                comboBox1.addItem(dbc.getFilename());
            });
            return null;
        });
        // 添加事件
        addEvent();
    }

    public boolean isAdd() {
        return isAdd;
    }

    private void initTable() {
        Panel panel1 = new Panel();
        panel1.setLayout(new BorderLayout());
        JScrollPane scrollPane1 = new JScrollPane(messageTable);
        panel1.add(scrollPane1, BorderLayout.CENTER);
        panel1.add(messageTable.getFilterPanel(), BorderLayout.NORTH);
        leftsplitPanel.setLeftComponent(panel1);

        Panel panel2 = new Panel();
        panel2.setLayout(new BorderLayout());
        JScrollPane scrollPane2 = new JScrollPane(signalTable);
        panel2.add(scrollPane2, BorderLayout.CENTER);
        panel2.add(signalTable.getFilterPanel(), BorderLayout.NORTH);
        leftsplitPanel.setRightComponent(panel2);

        Panel panel3 = new Panel();
        panel3.setLayout(new BorderLayout());
        JScrollPane scrollPane3 = new JScrollPane(signalInGroupTable);
        panel3.add(scrollPane3, BorderLayout.CENTER);
        centerSplitPanel.setRightComponent(panel3);
        JButton addButton = new JButton();
        addButton.setText("添加");
        addButton.setSize(40,100);
        panel3.add(addButton,BorderLayout.WEST);
        // 点击添加后的事件
        addButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                int[] selectedRows = signalTable.getSelectedRows();
                if (selectedRows.length == 0){
                    JOptionPane.showMessageDialog(null, "请选择要添加的信号");
                    return;
                }
                for (int selectedRow : selectedRows) {
                    DbcSignal dbcSignal = new DbcSignal();
                    if(!signalTable.getFilteredData().isEmpty()){
                        dbcSignal = signalTable.getFilteredData().get(selectedRow);
                    }else{
                        dbcSignal = signalTable.getDataAt(selectedRow);
                    }
                    SignalInGroup signalInGroup = new SignalInGroup();
                    BeanUtils.copyProperties(dbcSignal, signalInGroup);
                    signalInGroupTable.append(signalInGroup);
                }
            }
        });

    }


    private void addEvent() {
        // 下拉列表数据更改事件
        comboBox1.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                System.out.println("下拉列表数据更改事件");
                if (StringUtils.isBlank(Objects.requireNonNull(comboBox1.getSelectedItem()).toString())) {
                    System.out.println("请选择DBC文件");
                    return;
                }
                DatabaseManager dataBaseManager = new DatabaseManager();
                currentDbcMetaData = dataBaseManager.execute(DbcMetadata.class, (session) -> {
                    DbcMetadataMapper mapper = session.getMapper(DbcMetadataMapper.class);
                    QueryWrapper<DbcMetadata> wrapper = new QueryWrapper<>();
                    wrapper.lambda().eq(DbcMetadata::getFilename, comboBox1.getSelectedItem());
                    return mapper.selectOne(wrapper);
                });
                refreshMessageTable();
                centerSplitPanel.setDividerLocation(0.8);
            }
        });

        // messageTable 选中行事件
        messageTable.getSelectionModel().addListSelectionListener(new ListSelectionListener() {
            @Override
            public void valueChanged(ListSelectionEvent e) {
                List<DbcMessage> filteredData = messageTable.getFilteredData();
                DbcMessage rowMessage ;
                if(!filteredData.isEmpty()){
                    rowMessage = filteredData.get(messageTable.getSelectedRow());
                }else{
                    rowMessage = messageTable.getDataAt(messageTable.getSelectedRow());
                }
                // 获取message 包含的信号
                DatabaseManager dataBaseManager = new DatabaseManager();
                List<DbcSignal> dbcSignalList = dataBaseManager.execute(DbcSignal.class, (session) -> {
                    DbcSignalMapper mapper = session.getMapper(DbcSignalMapper.class);
                    QueryWrapper<DbcSignal> myWrapper = new QueryWrapper<>();
                    myWrapper.lambda().eq(DbcSignal::getMessageId, rowMessage.getMessageId())
                                    .eq(DbcSignal::getDbcMetadataId,rowMessage.getDbcMetadataId());
                    System.out.println("messageId:" + rowMessage.getMessageId());
                    return mapper.selectList(myWrapper);
                });
                signalTable.removeAll();
                signalTable.appendBatch(dbcSignalList);
            }
        });

        // OK 按钮点击事件
        okButton.addActionListener(new ActionListener() {

            @Override
            public void actionPerformed(ActionEvent e) {
                List<SignalInGroup> signalInGroupList = getSignalInGroupList();
                if(signalInGroupList== null || signalInGroupList.isEmpty()){
                    JOptionPane.showMessageDialog(null,"请选择要添加的数据");
                    return;
                }
                isAdd = true;
                if (selectionListener != null && isAdd()) {
                    selectionListener.onSignalsSelected(getSignalInGroupList());
                }
                dispose();
            }
        });
        // 监听窗口关闭事件
        addWindowListener(new WindowAdapter() {
            @Override
            public void windowClosing(java.awt.event.WindowEvent windowEvent) {
                System.out.println("窗口关闭事件");
                // 窗口关闭时触发回调
                if (selectionListener != null && isAdd()) {
                    selectionListener.onSignalsSelected(getSignalInGroupList());
                }
            }
        });
    }

    public List<SignalInGroup> getSignalInGroupList() {
        int[] selectedRows = signalInGroupTable.getSelectedRows();
        if (selectedRows.length == 0) {
            return new ArrayList<>();
        }
        List<SignalInGroup> signalInGroupList = new ArrayList<>();
        for (int selectedRow : selectedRows) {
            SignalInGroup signalInGroup = signalInGroupTable.getDataAt(selectedRow);
            signalInGroupList.add(signalInGroup);
        }
        return signalInGroupList;
    }

    private void refreshMessageTable() {
        System.out.println("刷新消息表");
        DatabaseManager dataBaseManager = new DatabaseManager();
        List<DbcMessage> messageList = dataBaseManager.execute(DbcMessage.class, (session) -> {
            DbcMessageMapper mapper = session.getMapper(DbcMessageMapper.class);
            QueryWrapper<DbcMessage> wrapper = new QueryWrapper<>();
            wrapper.lambda().eq(DbcMessage::getDbcMetadataId, currentDbcMetaData.getId());
            return mapper.selectList(wrapper);
        });
        messageTable.removeAll();
        messageTable.appendBatch(messageList);
    }

    private void initComponents() {
        // JFormDesigner - Component initialization - DO NOT MODIFY  //GEN-BEGIN:initComponents  @formatter:off
        dialogPane = new JPanel();
        contentPanel = new JPanel();
        topPanel = new JPanel();
        label1 = new JLabel();
        comboBox1 = new JComboBox();
        centerSplitPanel = new JSplitPane();
        leftsplitPanel = new JSplitPane();
        scrollPane3 = new JScrollPane();
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
                contentPanel.setLayout(new BorderLayout(5, 5));

                //======== topPanel ========
                {
                    topPanel.setLayout(new BoxLayout(topPanel, BoxLayout.X_AXIS));

                    //---- label1 ----
                    label1.setText("DBC\u6587\u4ef6");
                    topPanel.add(label1);

                    //---- comboBox1 ----
                    comboBox1.setMaximumRowCount(13);
                    topPanel.add(comboBox1);
                }
                contentPanel.add(topPanel, BorderLayout.NORTH);

                //======== centerSplitPanel ========
                {
                    centerSplitPanel.setOrientation(JSplitPane.VERTICAL_SPLIT);

                    //======== leftsplitPanel ========
                    {
                        leftsplitPanel.setResizeWeight(0.5);
                        leftsplitPanel.setOneTouchExpandable(true);
                        leftsplitPanel.setOrientation(JSplitPane.VERTICAL_SPLIT);
                    }
                    centerSplitPanel.setTopComponent(leftsplitPanel);
                    centerSplitPanel.setBottomComponent(scrollPane3);
                }
                contentPanel.add(centerSplitPanel, BorderLayout.CENTER);
            }
            dialogPane.add(contentPanel, BorderLayout.CENTER);

            //======== buttonBar ========
            {
                buttonBar.setBorder(new EmptyBorder(12, 0, 0, 0));
                buttonBar.setLayout(new GridBagLayout());
                ((GridBagLayout)buttonBar.getLayout()).columnWidths = new int[] {0, 85, 80};
                ((GridBagLayout)buttonBar.getLayout()).columnWeights = new double[] {1.0, 0.0, 0.0};

                //---- okButton ----
                okButton.setText("OK");
                buttonBar.add(okButton, new GridBagConstraints(1, 0, 1, 1, 0.0, 0.0,
                    GridBagConstraints.CENTER, GridBagConstraints.BOTH,
                    new Insets(0, 0, 0, 5), 0, 0));

                //---- cancelButton ----
                cancelButton.setText("Cancel");
                buttonBar.add(cancelButton, new GridBagConstraints(2, 0, 1, 1, 0.0, 0.0,
                    GridBagConstraints.CENTER, GridBagConstraints.BOTH,
                    new Insets(0, 0, 0, 0), 0, 0));
            }
            dialogPane.add(buttonBar, BorderLayout.SOUTH);
        }
        contentPane.add(dialogPane, BorderLayout.CENTER);
        pack();
        setLocationRelativeTo(getOwner());
        // JFormDesigner - End of component initialization  //GEN-END:initComponents  @formatter:on
    }

    // JFormDesigner - Variables declaration - DO NOT MODIFY  //GEN-BEGIN:variables  @formatter:off
    private JPanel dialogPane;
    private JPanel contentPanel;
    private JPanel topPanel;
    private JLabel label1;
    private JComboBox comboBox1;
    private JSplitPane centerSplitPanel;
    private JSplitPane leftsplitPanel;
    private JScrollPane scrollPane3;
    private JPanel buttonBar;
    private JButton okButton;
    private JButton cancelButton;
    // JFormDesigner - End of variables declaration  //GEN-END:variables  @formatter:on

    private EnhancedTable<DbcMessage> messageTable;
    private EnhancedTable<DbcSignal> signalTable;
    private EnhancedTable<SignalInGroup> signalInGroupTable;

    public static void main(String[] args) {
        AddSignalFromDbDialog dialog = new AddSignalFromDbDialog(new JFrame());
        dialog.pack();
        dialog.setVisible(true);
    }

}
