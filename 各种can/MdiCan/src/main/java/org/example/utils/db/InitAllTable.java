package org.example.utils.db;

import com.baomidou.mybatisplus.annotation.TableName;
import org.reflections.Reflections;

import java.util.Set;

public  class InitAllTable {

    public static void init() {
        Reflections reflections = new Reflections("org.example.pojo");
        Set<Class<?>> classes = reflections.getTypesAnnotatedWith(TableName.class);
        DatabaseManager dbManager = new DatabaseManager();
        for (Class<?> clazz : classes) {
            dbManager.registerEntity(clazz);
        }
    }
}
