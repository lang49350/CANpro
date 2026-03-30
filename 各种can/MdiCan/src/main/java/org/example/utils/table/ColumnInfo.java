package org.example.utils.table;

import lombok.Data;

@Data
public class ColumnInfo {
    private String name;
    Class<?> type;

    private  int index;

    private String verboserName;

    public ColumnInfo(String name, Class<?> type,int index) {
        this.name = name;
        this.type = type;
        this.index = index;
    }
}
