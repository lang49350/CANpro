package org.example.ui.project.chart.group;

import com.baomidou.mybatisplus.core.conditions.query.QueryWrapper;
import org.example.mapper.SignalGroupMapper;
import org.example.mapper.SignalInGroupMapper;
import org.example.pojo.dbc.DbcSignal;
import org.example.pojo.project.SignalGroup;
import org.example.pojo.project.SignalInGroup;
import org.example.ui.project.chart.freeChart.DraggableTimeChart;
import org.example.utils.db.DatabaseManager;
import org.example.utils.table.EnhancedTable;

import javax.swing.*;
import javax.swing.event.ListSelectionEvent;
import javax.swing.event.ListSelectionListener;
import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.util.ArrayList;
import java.util.List;

public class GroupMangeView extends JFrame {
    private EnhancedTable<SignalGroup> groupTable;
    private EnhancedTable<SignalInGroup> signalTable;
    private DatabaseManager dbManager;
    private List<SignalGroup> groups;
    private List<SignalInGroup> signals;

    private AddSignalFromDbDialog addSignalFromDbDialog;

    private SignalSelectionListener signalSelectionListener;

    // 用于别的模块中获取信号
    private SignalSelectionListener signalListener;

    // 提供者模式，true 代表从别的页面调用这个页面
    private boolean providerMode = false;


    // 隶属与哪个项目，后续根据项目号来查询数据
    private Long parentProjectId;

    // 作为提供者的时候显示
    private JButton selectSignalButton;

    // 调用这个页面的地址需要设置，用来获取这个页面关闭时选择的信号组数据
    public void setSignalSelectionListener(SignalSelectionListener listener) {
        this.signalListener = listener;
    }
    public void setProviderMode(boolean providerMode) {
        this.providerMode = providerMode;
    }
    public GroupMangeView(boolean providerMode) {
        this.providerMode = providerMode;
        dbManager = new DatabaseManager();
        groupTable = new EnhancedTable<>(SignalGroup.class);
        signalTable = new EnhancedTable<>(SignalInGroup.class);
        initComponents();
        loadGroups();
        setTitle("CAN信号管理系统");
        setSize(1000, 600);
        setLocationRelativeTo(null);
    }
    private void initComponents() {
        JSplitPane splitPane = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT);

        // 左侧面板：信号组管理
        JPanel groupPanel = new JPanel(new BorderLayout());
        groups = dbManager.execute(SignalGroup.class,session -> {
            SignalGroupMapper mapper = session.getMapper(SignalGroupMapper.class);
            return mapper.selectList(null);
        });
        groupTable.getSelectionModel().addListSelectionListener(new ListSelectionListener() {
            @Override
            public void valueChanged(ListSelectionEvent e) {
                if (!e.getValueIsAdjusting() && groupTable.getSelectedRow() != -1) {
                    loadSignals(groups.get(groupTable.getSelectedRow()).getId());
                }
            }
        });
        JScrollPane groupScrollPane = new JScrollPane(groupTable);
        groupPanel.add(groupScrollPane, BorderLayout.CENTER);

        JPanel groupButtonPanel = new JPanel();
        JButton addGroupButton = new JButton("添加组");
        JButton editGroupButton = new JButton("编辑组");
        JButton deleteGroupButton = new JButton("删除组");
        groupButtonPanel.add(addGroupButton);
        groupButtonPanel.add(editGroupButton);
        groupButtonPanel.add(deleteGroupButton);
        groupPanel.add(groupButtonPanel, BorderLayout.SOUTH);

        // 右侧面板：信号管理
        JPanel signalPanel = new JPanel(new BorderLayout());
        signals = new ArrayList<>();
        JScrollPane signalScrollPane = new JScrollPane(signalTable);
        signalPanel.add(signalScrollPane, BorderLayout.CENTER);

        JPanel signalButtonPanel = new JPanel();
        JButton addSignalButton = new JButton("手动添加信号");
        JButton addSignalFromDbButton = new JButton("从数据库添加信号");
        JButton editSignalButton = new JButton("编辑信号");
        JButton deleteSignalButton = new JButton("删除信号");
        selectSignalButton = new JButton("绘制这些信号");
//        JButton drawSignalButton = new JButton("立即绘制");
        signalButtonPanel.add(addSignalButton);
        signalButtonPanel.add(addSignalFromDbButton);
        signalButtonPanel.add(editSignalButton);
        signalButtonPanel.add(deleteSignalButton);
        if(providerMode){
            signalButtonPanel.add(selectSignalButton);
        }
//        signalButtonPanel.add(drawSignalButton);
        signalPanel.add(signalButtonPanel, BorderLayout.SOUTH);

        splitPane.setLeftComponent(groupPanel);
        splitPane.setRightComponent(signalPanel);
        splitPane.setDividerLocation(300);

        getContentPane().add(splitPane, BorderLayout.CENTER);

        //点击绘制这些信号后的工作
        selectSignalButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                int[] selectedRows = signalTable.getSelectedRows();
                if (selectedRows.length != 0) {
                    List<SignalInGroup> selectedSignals = new ArrayList<>();
                    for (int selectedRow : selectedRows){
                        SignalInGroup signalInGroup = signalTable.getDataAt(selectedRow);
                        selectedSignals.add(signalInGroup);
                    }
                    // 向调用该页面的页面传递数据
                    signalListener.onSignalsSelected(selectedSignals);
                    dispose();
                }
            }
        });

//        drawSignalButton.addActionListener(new ActionListener() {
//            @Override
//            public void actionPerformed(ActionEvent e) {
//                int selectedRow = groupTable.getSelectedRow();
//                if (selectedRow != -1) {
//                    // 向调用该页面的页面传递数据
//                    for (SignalInGroup signal : signals) {
//                        DraggableTimeChart chart = new DraggableTimeChart("信号绘制", Color.BLUE);
//                        chart.setVisible(true);
//                    }
//                }
//            }
//        });


         signalSelectionListener = signals -> {
             System.out.println("回调事件触发");
            if (signals != null && !signals.isEmpty()) {
                int groupId = groups.get(groupTable.getSelectedRow()).getId();
                for (SignalInGroup signal : signals) {
                    signal.setId(null);
                    signal.setGroupId(groupId);
                }
                signalTable.appendBatch(signals);
                // 存库
                dbManager.execute(SignalInGroup.class,session -> {
                    SignalInGroupMapper mapper = session.getMapper(SignalInGroupMapper.class);
                    return mapper.insertBatch(signals);
                });
                loadSignals(groupId);
            }
        };

        // 从数据库添加按钮
        addSignalFromDbButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                addSignalFromDbDialog = new AddSignalFromDbDialog(GroupMangeView.this);
                addSignalFromDbDialog.setSignalSelectionListener(signalSelectionListener);
                addSignalFromDbDialog.setVisible(true);
            }
        });

        // 添加组按钮事件
        addGroupButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                SignalGroupDialog dialog = new SignalGroupDialog(GroupMangeView.this, "添加信号组", true, null);
                dialog.setVisible(true);
                if (dialog.isOkClicked()) {
                    dbManager.execute(SignalGroup.class,session -> {
                        SignalGroupMapper mapper = session.getMapper(SignalGroupMapper.class);
                        return mapper.insert(dialog.getGroup());
                    });
                    loadGroups();
                }
            }
        });

        // 编辑组按钮事件
        editGroupButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                int selectedRow = groupTable.getSelectedRow();
                if (selectedRow == -1) {
                    JOptionPane.showMessageDialog(GroupMangeView.this, "请选择要编辑的信号组", "提示", JOptionPane.INFORMATION_MESSAGE);
                    return;
                }
                SignalGroup group = groups.get(selectedRow);
                SignalGroupDialog dialog = new SignalGroupDialog(GroupMangeView.this, "编辑信号组", true, group);
                dialog.setVisible(true);
                if (dialog.isOkClicked()) {
                    dbManager.execute(SignalGroup.class,session -> {
                        SignalGroupMapper mapper = session.getMapper(SignalGroupMapper.class);
                        return mapper.updateById(dialog.getGroup());
                    });
                    loadGroups();
                    if (groupTable.getSelectedRow() != -1) {
                        loadSignals(groups.get(groupTable.getSelectedRow()).getId());
                    }
                }
            }
        });

        // 删除组按钮事件
        deleteGroupButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                int selectedRow = groupTable.getSelectedRow();
                if (selectedRow == -1) {
                    JOptionPane.showMessageDialog(GroupMangeView.this, "请选择要删除的信号组", "提示", JOptionPane.INFORMATION_MESSAGE);
                    return;
                }
                SignalGroup group = groups.get(selectedRow);
                int confirm = JOptionPane.showConfirmDialog(GroupMangeView.this,
                        "确定要删除信号组 '" + group.getName() + "' 及其所有信号吗？",
                        "确认删除", JOptionPane.YES_NO_OPTION);
                if (confirm == JOptionPane.YES_OPTION) {
                    dbManager.execute(SignalGroup.class,session -> {
                        SignalGroupMapper mapper = session.getMapper(SignalGroupMapper.class);
                        return mapper.deleteById(group);
                    });
                    loadGroups();
                    signalTable.removeAll();
                }
            }
        });

        // 添加信号按钮事件
        addSignalButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                int selectedGroupRow = groupTable.getSelectedRow();
                if (selectedGroupRow == -1) {
                    JOptionPane.showMessageDialog(GroupMangeView.this, "请先选择一个信号组", "提示", JOptionPane.INFORMATION_MESSAGE);
                    return;
                }
                SignalDialog dialog = new SignalDialog(GroupMangeView.this, "添加信号", true, null, groups);
                dialog.setVisible(true);
                if (dialog.isOkClicked()) {
                    dbManager.execute(SignalInGroup.class,session -> {
                        SignalInGroupMapper mapper = session.getMapper(SignalInGroupMapper.class);
                        return mapper.insert(dialog.getMysignal());
                    });
                    loadSignals(groups.get(selectedGroupRow).getId());
                }
            }
        });

        // 编辑信号按钮事件
        editSignalButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                int selectedSignalRow = signalTable.getSelectedRow();
                if (selectedSignalRow == -1) {
                    JOptionPane.showMessageDialog(GroupMangeView.this, "请选择要编辑的信号", "提示", JOptionPane.INFORMATION_MESSAGE);
                    return;
                }
                SignalInGroup signal = signalTable.getDataAt(selectedSignalRow);
                SignalDialog dialog = new SignalDialog(GroupMangeView.this, "编辑信号", true, signal, groups);
                dialog.setVisible(true);
                if (dialog.isOkClicked()) {
                    dbManager.execute(SignalInGroup.class,session -> {
                        SignalInGroupMapper mapper = session.getMapper(SignalInGroupMapper.class);
                        return mapper.updateById(dialog.getMysignal());
                    });
                    loadSignals(groups.get(groupTable.getSelectedRow()).getId());
                }
            }
        });

        // 删除信号按钮事件
        deleteSignalButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                int[] selectedSignalRow = signalTable.getSelectedRows();
                if (selectedSignalRow .length == 0) {
                    JOptionPane.showMessageDialog(GroupMangeView.this, "请选择要删除的信号",
                            "提示", JOptionPane.INFORMATION_MESSAGE);
                    return;
                }
                List<SignalInGroup> toDels = new ArrayList<>();
                for (int i : selectedSignalRow) {
                    toDels.add(signalTable.getDataAt(i));
                }
                int confirm = JOptionPane.showConfirmDialog(GroupMangeView.this,
                        "确定要删除信号 '"  + "' 吗？",
                        "确认删除", JOptionPane.YES_NO_OPTION);
                if (confirm == JOptionPane.YES_OPTION) {
                    dbManager.execute(SignalInGroup.class,session -> {
                        SignalInGroupMapper mapper = session.getMapper(SignalInGroupMapper.class);
                        return mapper.deleteBatchIds(toDels);
                    });
                    loadSignals(groups.get(groupTable.getSelectedRow()).getId());
                }
            }
        });

        // 页面关闭事件
        addWindowListener(new WindowAdapter() {
            @Override
            public void windowClosing(WindowEvent e) {
                int selectedRow = groupTable.getSelectedRow();
                if (selectedRow != -1 && signalListener!=null) {
                    // 向调用该页面的页面传递数据
                    signalListener.onSignalsSelected(signals);
                }
                dispose();
            }
        });
    }

    private void loadGroups() {
        groups = dbManager.execute(SignalGroup.class,session -> {
            SignalGroupMapper mapper = session.getMapper(SignalGroupMapper.class);
            return mapper.selectList(null);
        });
        groupTable.removeAll();
        groupTable.appendBatch(groups);
    }

    private void loadSignals(int groupId) {
        signals = dbManager.execute(SignalInGroup.class,session -> {
            SignalInGroupMapper mapper = session.getMapper(SignalInGroupMapper.class);
            QueryWrapper<SignalInGroup> wrapper = new QueryWrapper<>();
            wrapper.lambda().eq(SignalInGroup::getGroupId, groupId);
            return mapper.selectList(wrapper);
        });
        signalTable.removeAll();
        signalTable.appendBatch(signals);
    }

    public List<SignalManager.SignalGroupWithSignals> getGroupsWithSignals() {
        return SignalManager.getInstance().getAllGroupsWithSignals();
    }

    public static void main(String[] args) {
        SwingUtilities.invokeLater(new Runnable() {
            @Override
            public void run() {
                GroupMangeView canSignalManagerUI = new GroupMangeView(false);
                canSignalManagerUI.setVisible(true);

            }
        });
    }
}