package org.example.utils.db;

import com.baomidou.mybatisplus.core.MybatisConfiguration;
import com.baomidou.mybatisplus.core.MybatisXMLLanguageDriver;
import com.baomidou.mybatisplus.extension.plugins.MybatisPlusInterceptor;
import com.baomidou.mybatisplus.extension.plugins.inner.DynamicTableNameInnerInterceptor;
import com.baomidou.mybatisplus.extension.plugins.inner.PaginationInnerInterceptor;
import com.baomidou.mybatisplus.extension.spring.MybatisSqlSessionFactoryBean;
import org.apache.ibatis.session.ExecutorType;
import org.apache.ibatis.session.SqlSession;
import org.apache.ibatis.session.SqlSessionFactory;
import org.apache.ibatis.type.JdbcType;
import org.example.annotation.MapperPath;
import org.example.utils.db.EntityTableGenerator;
import org.example.utils.db.SqlSessionCallback;

import javax.sql.DataSource;
import java.io.File;
import java.io.IOException;
import java.lang.reflect.Field;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;
import java.util.HashSet;

/**
 * 数据库管理器 - 优化会话管理，支持自动建表和动态数据源切换
 */
public class DatabaseManager {
    private Map<String, SqlSessionFactory> sessionFactoryMap = new HashMap<>();
    private Set<Class<?>> registeredEntities = new HashSet<>();

    /**
     * 执行数据库操作，自动管理会话生命周期
     * @param entityClass 实体类，用于确定数据库
     * @param callback 数据库操作回调
     * @param <T> 操作结果类型
     * @return 操作结果
     */
    public <T> T execute(Class<?> entityClass, SqlSessionCallback<T> callback) {
        String dbName = EntityTableGenerator.getDatabaseName(entityClass);

        String mapperPackage = getMapperPackage(entityClass);

        registerEntity(entityClass); // 传递正确的Mapper包路径

        try (SqlSession session = getSqlSession(dbName)) {
            return callback.doInSqlSession(session);
        } catch (Exception e) {
            throw new RuntimeException("数据库操作失败", e);
        }
    }

    /**
     * 增强版execute方法，支持自定义事务和执行器类型
     */
    public <T> T execute(Class<?> entityClass, SqlSessionCallback<T> callback,
                         boolean autoCommit, ExecutorType executorType) {
        String dbName = EntityTableGenerator.getDatabaseName(entityClass);
        String mapperPackage = getMapperPackage(entityClass);
        registerEntity(entityClass);

        // 关键：指定执行器类型和是否自动提交
        try (SqlSession session = getSqlSession(dbName, autoCommit, executorType)) {
            return callback.doInSqlSession(session);
        } catch (Exception e) {
            throw new RuntimeException("数据库操作失败", e);
        }
    }

    /**
     * 检查表是否存在
     * @param dbName 数据库名称
     * @param tableName 表名
     * @return 存在返回true，否则返回false
     */
    public boolean tableExists(String tableName,Class<?> entityClass) {
        String dbName = EntityTableGenerator.getDatabaseName(entityClass);
        String mapperPackage = getMapperPackage(entityClass);
        registerEntity(entityClass);
        String checkSql = "SELECT name FROM sqlite_master WHERE type='table' AND name = '?'";
        // 填充tablename 到sql
        try (SqlSession session = getSqlSession(dbName)) {
            // 填充tablename 到sql
            checkSql = checkSql.replace("?", tableName);
            // 使用原生SQL查询表是否存在
            return session.getConnection().prepareStatement(checkSql).executeQuery()
                    .next(); // 如果有结果，说明表存在
        } catch (Exception e) {
            System.err.println("检查表是否存在失败: " + tableName + ", 错误: " + e.getMessage());
            return false; // 异常时默认表不存在（防止建表失败）
        }
    }

    private static String getMapperPackage(Class<?> entityClass) {
        // 从注解获取Mapper路径
        MapperPath mapperPathAnnotation = entityClass.getAnnotation(MapperPath.class);
        String mapperPackage = mapperPathAnnotation != null ?
                mapperPathAnnotation.value() :
                "org.example.mapper"; // 默认包路径，根据您的项目调整
        return mapperPackage;
    }


    /**
     * 注册实体类（public修饰，允许外部调用）
     */
    public void registerEntity(Class<?> entityClass) {
        if (registeredEntities.contains(entityClass)) {
            return;
        }
        registeredEntities.add(entityClass);
        String dbName = EntityTableGenerator.getDatabaseName(entityClass);
        initDatabase(dbName);

        // 生成建表SQL并创建表
        String createTableSql = EntityTableGenerator.generateCreateTableSql(entityClass);
        createTable(dbName, createTableSql);

        // 动态添加Mapper（使用指定的包路径）
        addMapper(entityClass, getMapperPackage(entityClass));
    }

    /**
     * 动态添加Mapper接口
     */
    private void addMapper(Class<?> entityClass, String mapperPackage) {
        String entitySimpleName = entityClass.getSimpleName();
        String mapperName = mapperPackage + "." + entitySimpleName + "Mapper";

        System.out.println("正在添加Mapper: " + mapperName);

        try {
            Class<?> mapperClass = Class.forName(mapperName);
            MybatisConfiguration configuration = getCurrentConfiguration();
            configuration.addMapper(mapperClass);
        } catch (ClassNotFoundException e) {
            System.err.println("未找到Mapper: " + mapperName);
        }
    }

    /**
     * 获取当前MybatisConfiguration
     */
    private MybatisConfiguration getCurrentConfiguration() {
        // 假设当前使用第一个数据库的配置，可根据需要调整
        if (sessionFactoryMap.isEmpty()) {
            throw new IllegalStateException("没有可用的数据库配置");
        }

        try {
            return (MybatisConfiguration) sessionFactoryMap.values().iterator().next().getConfiguration();
        } catch (Exception e) {
            throw new RuntimeException("获取Mybatis配置失败", e);
        }
    }



    /**
     * 初始化数据库连接
     */
    private void initDatabase(String dbName) {
        if (sessionFactoryMap.containsKey(dbName)) {
            return;
        }

        try {
            File dbFile = new File(dbName + ".db");
            if (!dbFile.exists()) {
                dbFile.createNewFile();
            }
            // 开启读写同步
            String url = "jdbc:sqlite:" + dbFile.getAbsolutePath() + "?journal_mode=WAL";
            DataSource dataSource = createDataSource(url);

            MybatisSqlSessionFactoryBean factory = new MybatisSqlSessionFactoryBean();
            factory.setDataSource(dataSource);

            // 1. 创建MyBatis-Plus拦截器并添加动态表名拦截器
            MybatisPlusInterceptor interceptor = new MybatisPlusInterceptor();
            DynamicTableNameInnerInterceptor dynamicInterceptor = new DynamicTableNameInnerInterceptor();
            // 注册你的动态表名处理器（针对"can_message"表）
            dynamicInterceptor.setTableNameHandler(new DynamicTableNameHandler("can_message"));
            interceptor.addInnerInterceptor(dynamicInterceptor);

            interceptor.addInnerInterceptor(new PaginationInnerInterceptor()); // 分页插件
            factory.setPlugins(interceptor);

            MybatisConfiguration configuration = new MybatisConfiguration();
            configuration.setDefaultScriptingLanguage(MybatisXMLLanguageDriver.class);
            configuration.setJdbcTypeForNull(JdbcType.NULL);

            // 动态添加Mapper（假设Mapper命名规范为Entity+Mapper）
            for (Class<?> entity : registeredEntities) {
                String mapperName = getMapperPackage(entity) + "." + entity.getSimpleName() + "Mapper";
                try {
                    Class<?> mapperClass = Class.forName(mapperName);
                    configuration.addMapper(mapperClass);
                } catch (ClassNotFoundException e) {
                    System.err.println("未找到Mapper: " + mapperName);
                }
            }

            factory.setConfiguration(configuration);
            sessionFactoryMap.put(dbName, factory.getObject());
        } catch (Exception e) {
            throw new RuntimeException("初始化数据库失败: " + dbName, e);
        }
    }

    /**
     * 创建数据库表
     */
    public void createTable(String dbName, String sql) {
        try (SqlSession session = getSqlSession(dbName)) {
            session.getConnection().createStatement().execute(sql);
            session.commit();
        } catch (Exception e) {
            throw new RuntimeException("创建表失败: " + sql, e);
        }
    }

    /**
     * 获取SqlSession（自动关闭由调用方管理）
     */
    private SqlSession getSqlSession(String dbName) {
        return sessionFactoryMap.get(dbName).openSession(true); // auto commit
    }

    private SqlSession getSqlSession(String dbName,boolean autoCommit,ExecutorType executorType) {
        return sessionFactoryMap.get(dbName).openSession(executorType,autoCommit); // auto commit
    }

    /**
     * 创建数据源
     */
    private DataSource createDataSource(String url) {
        return new org.apache.tomcat.jdbc.pool.DataSource() {{
            setDriverClassName("org.sqlite.JDBC");
            setUrl(url);
            setInitialSize(5);
            setMaxActive(10);
            setMaxIdle(5);
            setMinIdle(2);
        }};
    }
}

