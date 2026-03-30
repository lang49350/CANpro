package org.example.utils.db;

import org.apache.ibatis.annotations.InsertProvider;
import java.util.List;

public interface BatchInsertMapper<T> {

    @InsertProvider(type = BatchSqlProvider.class, method = "insertBatch")
    int insertBatch(List<T> list);
}