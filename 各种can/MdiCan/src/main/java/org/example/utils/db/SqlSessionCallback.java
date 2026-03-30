package org.example.utils.db;

import org.apache.ibatis.session.SqlSession;

/**
 * 数据库操作回调接口（自定义函数式接口）
 */

@FunctionalInterface
public interface SqlSessionCallback<T> {
    T doInSqlSession(SqlSession session);
}