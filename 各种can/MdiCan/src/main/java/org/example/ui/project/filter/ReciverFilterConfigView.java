/*
 * Created by JFormDesigner on Tue May 20 13:01:20 CST 2025
 */

package org.example.ui.project.filter;

import com.baomidou.mybatisplus.core.conditions.query.QueryWrapper;
import org.apache.commons.lang3.StringUtils;
import org.example.mapper.MessageFilterMapper;
import org.example.pojo.filter.MessageFilter;
import org.example.utils.db.DatabaseManager;
import org.example.utils.event.EventManager;
import org.example.utils.event.EventString;

import javax.swing.*;
import javax.swing.table.DefaultTableModel;
import java.awt.*;
import java.awt.event.ActionEvent;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.List;

/**
 * @author jingq
 */
public class ReciverFilterConfigView extends JFrame {
    private DefaultTableModel model;

    // 所属项目的ID
    private Long projectId;
    public ReciverFilterConfigView(Long projectId) {
        this.projectId = projectId;
        initComponents();
        model = new DefaultTableModel(new Object[][]{
                {1, 2, 2, 2, 2}
        },new String[]{
                "id", "creater", "createTime", "MessageIds:16进制，0x前缀可不加", "enabled:Y/N"
        });
        table.setModel(model);
        table.setRowHeight(30);
        loadData();
        // 设置窗体最大话
        setExtendedState(JFrame.MAXIMIZED_BOTH);
    }

    private void loadData() {
        model.setRowCount(0);
        DatabaseManager databaseManager = new DatabaseManager();
        List<MessageFilter> messageFilters = databaseManager.execute(MessageFilter.class, session->{
            MessageFilterMapper mapper = session.getMapper(MessageFilterMapper.class);
            QueryWrapper<MessageFilter> wrapper = new QueryWrapper<>();
            wrapper.lambda().eq(MessageFilter::getProject_id, projectId);
            return mapper.selectList(wrapper);
        });

        for (MessageFilter messageFilter : messageFilters) {
            String messageIdsStr = messageFilter.getMessageIds();
            model.addRow(new Object[]{messageFilter.getId(),
                    messageFilter.getCreater(),
                    messageFilter.getCreateTime(),
                    messageIdsStr,
                    messageFilter.getEnabled()});
        }
    }

    private void addBtnHandler(ActionEvent e) {
        // TODO add your code here
        System.out.println("add new data");
        // 新增一行空数据
        Date date = new Date();
        // 格式化未yyyy-MM-dd HH:mm:ss
        SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
        String dateStr = sdf.format(date);
        model.addRow(new Object[]{null,null,dateStr,null,"Y"});
        table.setRowSelectionInterval(table.getRowCount()-1,table.getRowCount()-1);

    }

    private void editBtnHandler(ActionEvent e) {
        // TODO add your code here
        // 获取当前行
        int selectedRow = table.getSelectedRow();
        if(selectedRow != -1){
           // 获取行数据
            Object[] selectedRowData = getSelectedRowData(selectedRow);
            System.out.println("edit data is " + Arrays.toString(selectedRowData));
            // 转换为MessageFilter
            MessageFilter messageFilter = new MessageFilter();
            messageFilter.setProject_id(projectId);
            for (int i = 0; i < selectedRowData.length; i++) {
                if(i == 0){
                    Object idCol = selectedRowData[i];
                    if( idCol!=null && StringUtils.isNotBlank(idCol.toString())){
                        messageFilter.setId(Integer.parseInt(idCol.toString()));
                    }else{
                        messageFilter.setId(null);
                    }

                }else if(i == 1){
                    if(selectedRowData[i] == null){
                        messageFilter.setCreater("");
                    }else{
                        messageFilter.setCreater((String) selectedRowData[i]);
                    }
                }else if(i == 2){
                    messageFilter.setCreateTime((String) selectedRowData[i]);
                }else if(i == 3){
                    if(selectedRowData[i] == null){
                        JOptionPane.showMessageDialog(this, "messageIds is null");
                        throw new RuntimeException("messageIds is null");
                    }
                    String ids = selectedRowData[i].toString();
                    messageFilter.setMessageIds(ids);
                }else if(i == 4){
                    messageFilter.setEnabled(selectedRowData[i].toString());
                }
            }
            try {
                DatabaseManager databaseManager = new DatabaseManager();
                databaseManager.execute(MessageFilter.class, session->{
                    MessageFilterMapper mapper = session.getMapper(MessageFilterMapper.class);
                    if(messageFilter.getId() == null){
                       return mapper.insert(messageFilter);
                    }else {
                       return  mapper.updateById(messageFilter);
                    }
                });
                JOptionPane.showMessageDialog(this, "save success");
                loadData();
                EventManager.getInstance().notifyListeners(EventString.UPDATE_TABLE_FILTER_CONFIG.toString());
            }catch (Exception ex){
                ex.printStackTrace();
            }

        }
    }

    private Object[] getSelectedRowData(int selectedRow) {
        Object[] rowData = new Object[model.getColumnCount()];
        for (int i = 0; i < model.getColumnCount(); i++) {
            try {
                rowData[i] = model.getValueAt(selectedRow, i);
            }catch (Exception e){
                rowData[i] = null;
            }
        }
        return rowData;
    }

    private void delBtnHandler(ActionEvent e) {
        // TODO add your code here
        int selectedRow = table.getSelectedRow();
        if(selectedRow != -1){
            // 获取行数据
            Object[] selectedRowData = getSelectedRowData(selectedRow);
            System.out.println("del data is " + Arrays.toString(selectedRowData));
            // 获取Id
            Integer id = (Integer) selectedRowData[0];
            DatabaseManager databaseManager = new DatabaseManager();
            databaseManager.execute(MessageFilter.class, session->{
                MessageFilterMapper mapper = session.getMapper(MessageFilterMapper.class);
                return mapper.deleteById(id);
            });
            // 删除表格中的当前行
            model.removeRow(selectedRow);
            EventManager.getInstance().notifyListeners(EventString.UPDATE_TABLE_FILTER_CONFIG.toString());
        }
    }

    private void enableBtnHandler(ActionEvent e) {
        // TODO add your code here
        int selectedRow = table.getSelectedRow();
        if(selectedRow != -1){
            Object[] selectedRowData = getSelectedRowData(selectedRow);
            if(selectedRowData[4].toString().equals("Y")){
                selectedRowData[4] = "N";
            }else {
                selectedRowData[4] = "Y";
            }
            model.setValueAt(selectedRowData[4], selectedRow, 4);
            Object selectedRowDatum = selectedRowData[3];
            String messageIds = selectedRowDatum.toString();
            DatabaseManager databaseManager = new DatabaseManager();
            MessageFilter messageFilter = new MessageFilter((Integer) selectedRowData[0],
                    (String) selectedRowData[1],
                    (String) selectedRowData[2],
                    messageIds,
                    (String) selectedRowData[4],projectId);
            databaseManager.execute(MessageFilter.class, session->{
                MessageFilterMapper mapper = session.getMapper(MessageFilterMapper.class);
                return mapper.updateById(messageFilter);
            });
            loadData();
            EventManager.getInstance().notifyListeners(EventString.UPDATE_TABLE_FILTER_CONFIG.toString());

        }
    }

    // 获取当前有效的过滤器
    public List<MessageFilter> getCurrentMessageFilter(){
        // 获取所有数据
        DatabaseManager databaseManager = new DatabaseManager();
        return databaseManager.execute(MessageFilter.class, session->{
            MessageFilterMapper mapper = session.getMapper(MessageFilterMapper.class);
            QueryWrapper<MessageFilter> queryWrapper = new QueryWrapper<>();
            queryWrapper.lambda().eq(MessageFilter::getEnabled, "Y")
                    .eq(MessageFilter::getProject_id, projectId);
            return mapper.selectList(queryWrapper);
        });
    }

    // 合并取出所有有效过滤器中的messageId
    public List<String> getCurrentMessageIds(){
        List<String> messageIds = new ArrayList<>();
        List<String> ids = new ArrayList<>();
        for (MessageFilter messageFilter : getCurrentMessageFilter()) {
            messageIds.add(messageFilter.getMessageIds());
        }
        for (String messageId : messageIds) {
            ids.add(messageId.replace("0x","").replace("0X",""));
        }
        return ids;
    }

    private void initComponents() {
        // JFormDesigner - Component initialization - DO NOT MODIFY  //GEN-BEGIN:initComponents  @formatter:off
        panel1 = new JPanel();
        addBtn = new JButton();
        editBtn = new JButton();
        delBtn = new JButton();
        enableBtn = new JButton();
        scrollPane1 = new JScrollPane();
        table = new JTable();

        //======== this ========
        Container contentPane = getContentPane();
        contentPane.setLayout(new BorderLayout());

        //======== panel1 ========
        {
            panel1.setLayout(new GridLayout(1, 4, 20, 0));

            //---- addBtn ----
            addBtn.setText("\u65b0\u589e");
            addBtn.addActionListener(e -> addBtnHandler(e));
            panel1.add(addBtn);

            //---- editBtn ----
            editBtn.setText("\u4fdd\u5b58\u4fee\u6539");
            editBtn.addActionListener(e -> editBtnHandler(e));
            panel1.add(editBtn);

            //---- delBtn ----
            delBtn.setText("\u5220\u9664");
            delBtn.addActionListener(e -> delBtnHandler(e));
            panel1.add(delBtn);

            //---- enableBtn ----
            enableBtn.setText("\u5207\u6362\u6709\u6548\u72b6\u6001");
            enableBtn.addActionListener(e -> enableBtnHandler(e));
            panel1.add(enableBtn);
        }
        contentPane.add(panel1, BorderLayout.NORTH);

        //======== scrollPane1 ========
        {

            //---- table ----
            table.setModel(new DefaultTableModel(2, 0));
            scrollPane1.setViewportView(table);
        }
        contentPane.add(scrollPane1, BorderLayout.CENTER);
        pack();
        setLocationRelativeTo(getOwner());
        // JFormDesigner - End of component initialization  //GEN-END:initComponents  @formatter:on

    }

    // JFormDesigner - Variables declaration - DO NOT MODIFY  //GEN-BEGIN:variables  @formatter:off
    private JPanel panel1;
    private JButton addBtn;
    private JButton editBtn;
    private JButton delBtn;
    private JButton enableBtn;
    private JScrollPane scrollPane1;
    private JTable table;
    // JFormDesigner - End of variables declaration  //GEN-END:variables  @formatter:on
    public static void main(String[] args) {
        ReciverFilterConfigView view = new ReciverFilterConfigView(null);
        view.setVisible(true);
    }
}
