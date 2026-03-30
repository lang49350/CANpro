package org.example.utils.event;

import org.apache.poi.ss.formula.functions.T;
import org.example.pojo.data.SendMessage;
import org.example.pojo.dbc.DbcSignal;

// 定义点击标准表格中的编辑按钮的时候的动作
@FunctionalInterface
public interface TableEditCallback<T> {
    T onEdit(T t); // 定义回调参数
}
