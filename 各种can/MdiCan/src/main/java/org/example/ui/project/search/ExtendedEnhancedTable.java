package org.example.ui.project.search;

import org.example.pojo.data.CanMessage;
import org.example.utils.table.EnhancedTable;

import javax.swing.*;
import javax.swing.event.ListSelectionEvent;
import javax.swing.event.ListSelectionListener;
import java.awt.*;
import java.util.logging.Logger;

public class ExtendedEnhancedTable<T> extends EnhancedTable<T> {
    private static final Logger logger = Logger.getLogger(ExtendedEnhancedTable.class.getName());

    // 可以添加新的监听器接口用于扩展操作的回调
    public interface RowSelectionListener {
        /**
         * 当选中行发生变化时触发
         * @param rowIndex 选中的行索引（-1表示无选中行）
         * @param rowData 选中行的数据（null表示无选中行）
         */
        void onRowSelected(int rowIndex, Object rowData);
    }

    private RowSelectionListener selectionListener;
    public ExtendedEnhancedTable(Class entityClass,RowSelectionListener listener) {
        super(entityClass);
        this.selectionListener = listener;
        initSelectionListener();
    }

    // 初始化选择监听器
    private void initSelectionListener() {
        // 获取表格的选择模型
        ListSelectionModel selectionModel = getSelectionModel();

        // 添加选择事件监听器
        selectionModel.addListSelectionListener(new ListSelectionListener() {
            @Override
            public void valueChanged(ListSelectionEvent e) {
                // 防止事件重复触发
                if (e.getValueIsAdjusting()) {
                    return;
                }

                // 获取当前选中的行索引
                int selectedRow = getSelectedRow();

                // 获取选中行的数据
                Object rowData = null;
                if (selectedRow != -1 ) {
                    rowData = getTableData().get(selectedRow);
                }
                // 触发自定义选中事件
                if (selectionListener != null) {
                    selectionListener.onRowSelected(selectedRow, rowData);
                }
            }
        });
    }

    /**
     * 设置选中行事件监听器
     */
    public void setRowSelectionListener(RowSelectionListener listener) {
        this.selectionListener = listener;
    }

    /**
     * 根据行号滚动到指定行并选中该行
     * @param rowIndex 要定位的行索引（基于过滤后的数据索引）
     */
    public void scrollToAndSelectRow(int rowIndex) {
        // 检查行索引有效性
        if (rowIndex < 0) {
            return;
        }

        // 在EDT线程中执行UI操作
        SwingUtilities.invokeLater(() -> {
            // 清除之前的选择
            clearSelection();

            // 选中指定行
            setRowSelectionInterval(rowIndex, rowIndex);

            // 滚动到指定行
            Rectangle cellRect = getCellRect(rowIndex, 0, true);
            if (cellRect != null) {
                scrollRectToVisible(cellRect);
            }

            // 确保表格获得焦点
            requestFocusInWindow();
        });
    }

}
