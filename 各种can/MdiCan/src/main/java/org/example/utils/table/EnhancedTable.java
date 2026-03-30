package org.example.utils.table;

import lombok.AllArgsConstructor;
import lombok.Getter;
import lombok.NoArgsConstructor;
import lombok.Setter;
import org.apache.commons.lang3.StringUtils;
import org.apache.poi.ss.formula.functions.Column;
import org.example.annotation.AsColumn;
import org.example.annotation.VerboserName;
import org.example.pojo.data.CanMessage;
import org.example.pojo.data.SendMessage;
import org.example.utils.event.TableDataEditCallback;
import org.example.utils.event.TableEditCallback;
import org.example.utils.web.UdpWifiDataReceiver;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.swing.*;
import javax.swing.event.DocumentEvent;
import javax.swing.event.DocumentListener;
import javax.swing.table.AbstractTableModel;
import java.awt.*;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.util.*;
import java.util.List;
import java.util.concurrent.*;
import java.util.function.Predicate;
import java.util.stream.Collectors;

@AllArgsConstructor
@Getter
@Setter
public class EnhancedTable<T> extends JTable {
    private static final Logger logger = LoggerFactory.getLogger(EnhancedTable.class);
    private final EnhancedTableModel model;
    private final Class<T> entityClass;
//    private final List<T> data = new ArrayList<>();
    private final ConcurrentLinkedQueue<T> data = new ConcurrentLinkedQueue<>();
    private final ExecutorService executor = Executors.newSingleThreadExecutor();
    private final Map<String, ColumnInfo> columnInfoMap = new LinkedHashMap<>();
    private List<T> filteredData = new ArrayList<>();
    private final Map<String, Predicate<T>> columnFilters = new HashMap<>();
    private String sortColumn = null;
    private boolean sortAscending = true;
    private JTextField[] filterFields;
    private JPanel filterPanel;
    private List<T> changedList = new ArrayList<>();
    // 排除字段
    private List<String> unShowColumns = new ArrayList<>();
    // 是否使用别名
    private boolean useVerboserName = false;

    //是否开启编辑检查
    private boolean enableEditCheck = false;
    // 编辑检查的回调函数
    private TableDataEditCallback editCheckCallback = null;
    // 定义编辑的回调
    private TableEditCallback<T> editCallback = null;

    public List<T> getData() {
        return new ArrayList<>(data);
    }

    public EnhancedTable(Class<T> entityClass) {
        this.entityClass = entityClass;
        this.model = new EnhancedTableModel();
        setModel(model);
        initColumns();
        setAutoCreateRowSorter(true);
        createFilterPanel();
        addDoubleClickListener();
    }

    public void initColumns() {
        columnInfoMap.clear();
        for (java.lang.reflect.Field field : entityClass.getDeclaredFields()) {
            String fieldName = field.getName();
            // 检查是否在排除清单
            if (unShowColumns.contains(fieldName)) {continue;}
            Class<?> fieldType = field.getType();
            AsColumn asColumn = field.getAnnotation(AsColumn.class);
            VerboserName verboserName = field.getAnnotation(VerboserName.class);
            if (asColumn != null) {
                String index = asColumn.value();
                columnInfoMap.put(fieldName, new ColumnInfo(fieldName, fieldType,Integer.parseInt(index)));
                if(verboserName != null){
                   columnInfoMap.get(fieldName).setVerboserName(verboserName.value());
                }
            }
        }
        model.fireTableStructureChanged();
    }

    public void fireChange(){
        model.fireTableStructureChanged();
    }

    private void createFilterPanel() {
        filterPanel = new JPanel(new GridLayout(1, 0));
        filterFields = new JTextField[columnInfoMap.size()];

        int i = 0;
        for (String columnName : columnInfoMap.keySet()) {
            final String colName = columnName;
            JPanel panel = new JPanel(new BorderLayout());

            JTextField filterField = new JTextField();
            filterFields[i++] = filterField;

            filterField.getDocument().addDocumentListener(new DocumentListener() {
                @Override
                public void insertUpdate(DocumentEvent e) {
                    applyFilter(colName, filterField.getText());
                }

                @Override
                public void removeUpdate(DocumentEvent e) {
                    applyFilter(colName, filterField.getText());
                }

                @Override
                public void changedUpdate(DocumentEvent e) {
                    applyFilter(colName, filterField.getText());
                }
            });

            panel.add(filterField, BorderLayout.CENTER);

            JButton clearButton = new JButton("X");
            clearButton.setPreferredSize(new Dimension(25, 25));
            clearButton.addActionListener(e -> filterField.setText(""));
            panel.add(clearButton, BorderLayout.EAST);

            filterPanel.add(panel);
        }
    }

    /**
     * 根据行号滚动到指定行并选中该行
     * @param rowIndex 要定位的行索引（基于过滤后的数据索引）
     */
    public void scrollToAndSelectRow(int rowIndex) {
        // 检查行索引有效性
        if (rowIndex < 0 || rowIndex >= filteredData.size()) {
            logger.warn("无效的行索引: {}", rowIndex);
            return;
        }
        SwingUtilities.invokeLater(() -> {
            // 清除之前的选择
            clearSelection();

            // 选中指定行
            setRowSelectionInterval(rowIndex, rowIndex);

            // 获取表格的滚动面板和视口
            JScrollPane scrollPane = (JScrollPane) SwingUtilities.getAncestorOfClass(JScrollPane.class, this);
            if (scrollPane == null) {
                return;
            }

            JViewport viewport = scrollPane.getViewport();
            Rectangle cellRect = getCellRect(rowIndex, 0, true);

            if (viewport != null) {
                // 获取视口可见区域
                Rectangle viewRect = viewport.getViewRect();
                // 计算视口高度的一半（目标中点位置）
                int viewportHalfHeight = viewRect.height / 2;
                // 计算行的中点位置
                int rowMidY = cellRect.y + (cellRect.height / 2);
                // 计算需要滚动的位置，使行中点与视口中点对齐
                int targetY = rowMidY - viewportHalfHeight;

                // 确保滚动位置不会为负数
                targetY = Math.max(0, targetY);

                // 设置新的滚动位置
                viewRect.setLocation(viewRect.x, targetY);
                scrollRectToVisible(viewRect);
            }

            // 确保表格获得焦点
            requestFocusInWindow();
        });
    }


    public void setColumnFilter(String columnName, String value){
        applyFilter(columnName, value);
    }

    private void applyFilter(String columnName, String value) {
        executor.submit(() -> {
            synchronized (data) {
                if (columnInfoMap.containsKey(columnName)) {
                    if (value == null || value.isEmpty()) {
                        columnFilters.remove(columnName);
                    } else {
                        columnFilters.put(columnName, entity -> {
                            try {
                                java.lang.reflect.Field field = entityClass.getDeclaredField(columnName);
                                field.setAccessible(true);
                                Object fieldValue = field.get(entity);
                                if (fieldValue == null) {
                                    return false;
                                }
                                return fieldValue.toString().toLowerCase().contains(value.toLowerCase());
                            } catch (Exception e) {
                                return false;
                            }
                        });
                    }
                    applyAllFiltersAndSort();
                    model.fireTableDataChanged();
                }
            }
        });
    }

    private void applyAllFiltersAndSort() {
        // 应用所有筛选条件
        filteredData = data.stream()
                .filter(entity -> {
                    for (Predicate<T> filter : columnFilters.values()) {
                        if (!filter.test(entity)) {
                            return false;
                        }
                    }
                    return true;
                }).collect(Collectors.toList());

        // 应用排序
        if (sortColumn != null && !sortColumn.isEmpty()) {
            try {
                java.lang.reflect.Field field = entityClass.getDeclaredField(sortColumn);
                field.setAccessible(true);
                Comparator<T> comparator = (e1, e2) -> {
                    try {
                        Object v1 = field.get(e1);
                        Object v2 = field.get(e2);
                        if (v1 instanceof Comparable && v2 instanceof Comparable) {
                            return ((Comparable) v1).compareTo(v2);
                        }
                        return 0;
                    } catch (Exception ex) {
                        return 0;
                    }
                };
                filteredData = filteredData.stream()
                        .sorted(sortAscending ? comparator : comparator.reversed())
                        .collect(Collectors.toList());
            } catch (Exception e) {
                // 忽略排序错误
            }
        }
    }

    public List<T> getTableData() {
        return new ArrayList<>(data);
    }

    public T getDataAt(long index) {
        if (index < 0 || index >= data.size()) {
            return null;
        }
//        return data.get((int) index);
        return getDataByIndex(index);
    }

    private T getDataByIndex(long index){
        if (index < 0 || index >= data.size()) {
            return null;
        }
        Iterator<T> iterator = data.iterator();
        int currentIndex = 0;
        while (iterator.hasNext()) {
            T element = iterator.next();
            if (currentIndex == index) {
                return element; // 找到目标索引，返回元素
            }
            currentIndex++;
        }

        // 索引超出队列长度
        return null;
    }

    public void append(T entity) {
        executor.submit(() -> {
            synchronized (data) {
                data.add(entity);
                applyAllFiltersAndSort();
                model.fireTableDataChanged();
            }
        });
    }

    public void appendBatch(List<T> entities) {
        executor.submit(() -> {
            synchronized (data) {
                data.addAll(entities);
                applyAllFiltersAndSort();
                model.fireTableDataChanged();
            }
        });
    }


    public void appendBatchNoSort(List<T> entities) {
        executor.submit(() -> {
//            logger.info("表格开始批量插入数据"  + entities.size());
            synchronized (data) {
                data.addAll(entities);
                // copy data to fiteredData
                filteredData.clear();
                filteredData.addAll(data);
                model.fireTableDataChanged();
                scrollRectToVisible(getCellRect(getRowCount() - 1, 0, true));
            }
//            logger.info("表格开始批量插入数据结束,数据条数 " + data.size());
        });
    }

    // EnhancedTable.java 中新增
    public void waitForTasks() throws InterruptedException {
        // 用于标记所有任务完成的信号量
        CountDownLatch latch = new CountDownLatch(1);

        // 向单线程池提交一个"监控任务"，它会在所有之前的任务完成后执行
        // 当此任务执行时，说明前面所有任务已完成
        executor.submit(latch::countDown);

        // 阻塞等待监控任务执行（即所有前置任务完成）
        // 可添加超时参数避免无限等待：latch.await(1, TimeUnit.MINUTES)
        latch.await(1, TimeUnit.MINUTES);
    }


    public int getRowCount() {
        return filteredData.size();
    }

    public void removeDataAt(long index) {
        executor.submit(() -> {
            synchronized (data) {
                if (index >= 0 && index < data.size()) {
                    data.remove((int) index);
                    applyAllFiltersAndSort();
                    model.fireTableDataChanged();
                }
            }
        });
    }

    public void removeAll() {
        executor.submit(() -> {
            synchronized (data) {
                data.clear();
                try {
                    filteredData.clear();
                } catch (Exception e) {
                    e.printStackTrace();
                }

                model.fireTableDataChanged();
            }
        });
    }

    public void sortByColumn(String columnName, boolean ascending) {
        executor.submit(() -> {
            synchronized (data) {
                if (columnInfoMap.containsKey(columnName)) {
                    this.sortColumn = columnName;
                    this.sortAscending = ascending;
                    applyAllFiltersAndSort();
                    model.fireTableDataChanged();
                }
            }
        });
    }

    public JPanel getFilterPanel() {
        return filterPanel;
    }


    private class EnhancedTableModel extends AbstractTableModel {
        @Override
        public int getRowCount() {
            return filteredData.size();
        }

        @Override
        public int getColumnCount() {
            return columnInfoMap.size();
        }

        @Override
        public Object getValueAt(int rowIndex, int columnIndex) {
            if (rowIndex < 0 || rowIndex >= filteredData.size()) {
                return null;
            }
            T entity = filteredData.get(rowIndex);
            List<String> columnNames = new ArrayList<>(columnInfoMap.keySet());
            if (columnIndex < 0 || columnIndex >= columnNames.size()) {
                return null;
            }
            String columnName = columnNames.get(columnIndex);
            try {
                java.lang.reflect.Field field = entityClass.getDeclaredField(columnName);
                field.setAccessible(true);
                return field.get(entity);
            } catch (Exception e) {
                return null;
            }
        }

        @Override
        public String getColumnName(int column) {
            List<String> columnNames = new ArrayList<>(columnInfoMap.keySet());
            if (column < 0 || column >= columnNames.size()) {
                return "";
            }
             //return columnNames.get(column);
            String columnName = columnNames.get(column);
            ColumnInfo columnInfo = columnInfoMap.get(columnName);
            if(!useVerboserName){
                return columnInfo != null ? columnInfo.getName() : columnName;
            }else{
                if(columnInfo != null && columnInfo.getVerboserName() != null){
                    return columnInfo.getVerboserName();
                }else{
                    return columnName;
                }
            }

        }

        @Override
        public Class<?> getColumnClass(int columnIndex) {
            List<ColumnInfo> columnInfos = new ArrayList<>(columnInfoMap.values());
            if (columnIndex < 0 || columnIndex >= columnInfos.size()) {
                return Object.class;
            }
            return columnInfos.get(columnIndex).type;
        }
    }


    public void shutdown() {
        executor.shutdown();
        try {
            if (!executor.awaitTermination(5, TimeUnit.SECONDS)) {
                executor.shutdownNow();
            }
        } catch (InterruptedException e) {
            executor.shutdownNow();
        }
    }

    // 添加双击监听器
    private void addDoubleClickListener() {
        addMouseListener(new MouseAdapter() {
            @Override
            public void mouseClicked(MouseEvent e) {
                if (e.getClickCount() == 2) {
                    int row = rowAtPoint(e.getPoint());
                    int col = columnAtPoint(e.getPoint());
                    if (row >= 0 && col >= 0) {
                        editCell(row, col);
                        T dataAt = getDataAt(row);
                        changedList.add(dataAt);
                    }
                }
            }
        });
    }

    // 编辑单元格数据
    private void editCell(int row, int col) {
        T entity = filteredData.get(row);
        // 检查编辑回调，如果有，就用设置好的，如果没有就使用默认的
        if(editCallback != null){
            T t = editCallback.onEdit(entity);
            //替换值
            filteredData.set(row,t);
        }else{
            defaultEditFunc(row, col, entity);
        }

    }

    private void defaultEditFunc(int row, int col, T entity) {
        List<String> columnNames = new ArrayList<>(columnInfoMap.keySet());
        String columnName = columnNames.get(col);
        try {
            java.lang.reflect.Field field = entityClass.getDeclaredField(columnName);
            field.setAccessible(true);
            Object oldValue = field.get(entity);

            // 获取单元格的屏幕位置
            Rectangle cellRect = getCellRect(row, col, true);
            Point cellLocation = new Point(cellRect.x, cellRect.y);
            SwingUtilities.convertPointToScreen(cellLocation, this);

            // 创建一个临时的 JFrame 作为对话框的父窗口
            JFrame tempFrame = new JFrame();
            tempFrame.setUndecorated(true); // 无边框
            tempFrame.setOpacity(0.0f);     // 完全透明
            tempFrame.setLocation(cellLocation);
            tempFrame.setVisible(true);

            // 显示对话框，并将临时 JFrame 作为父窗口
            String input = JOptionPane.showInputDialog(
                    tempFrame,
                    "请输入新值",
                    oldValue
            );

            // 关闭临时 JFrame
            tempFrame.dispose();

            if (input != null) {
                Class<?> fieldType = field.getType();
                Object newValue = convertToType(input, fieldType);
                field.set(entity, newValue);
                applyAllFiltersAndSort();
                model.fireTableDataChanged();
                // 如果开启了编辑检查，并且回调函数不为空，触发回调函数，并传递当前编辑的行和列，新旧值给回调函数
                if (enableEditCheck && editCheckCallback != null) {
                    editCheckCallback.onRulePassed(columnName, row, col, oldValue, newValue);
                }
            }
        } catch (Exception ex) {
            ex.printStackTrace();
        }
    }


    // 编辑单元格数据
//    private void editCell(int row, int col) {
//        T entity = filteredData.get(row);
//        List<String> columnNames = new ArrayList<>(columnInfoMap.keySet());
//        String columnName = columnNames.get(col);
//        try {
//            java.lang.reflect.Field field = entityClass.getDeclaredField(columnName);
//            field.setAccessible(true);
//            Object oldValue = field.get(entity);
//            String input = JOptionPane.showInputDialog(this, "请输入新值", oldValue);
//            if (input != null) {
//                Class<?> fieldType = field.getType();
//                Object newValue = convertToType(input, fieldType);
//                field.set(entity, newValue);
//                applyAllFiltersAndSort();
//                model.fireTableDataChanged();
//            }
//        } catch (Exception ex) {
//            ex.printStackTrace();
//        }
//    }

    // 将输入字符串转换为指定类型
    private Object convertToType(String input, Class<?> type) {
        if (type == String.class) {
            return input;
        } else if (type == Integer.class || type == int.class) {
            try {
                return Integer.parseInt(input);
            } catch (NumberFormatException e) {
                return 0;
            }
        } else if (type == Double.class || type == double.class) {
            try {
                return Double.parseDouble(input);
            } catch (NumberFormatException e) {
                return 0.0;
            }
        }
        return input;
    }
}