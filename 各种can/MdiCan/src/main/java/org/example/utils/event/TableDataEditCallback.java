package org.example.utils.event;


//定义了一个标准的表格编辑事件回调函数
@FunctionalInterface
public interface TableDataEditCallback {
    void onRulePassed(String columnName, int row ,int col,Object oldVal,Object newVal); // 定义回调参数
}