package org.example.utils.db;

import com.baomidou.mybatisplus.core.MybatisConfiguration;
import com.baomidou.mybatisplus.extension.plugins.MybatisPlusInterceptor;
import com.baomidou.mybatisplus.extension.plugins.handler.TableNameHandler;
import com.baomidou.mybatisplus.extension.plugins.inner.DynamicTableNameInnerInterceptor;
import com.baomidou.mybatisplus.extension.spring.MybatisSqlSessionFactoryBean;
import org.apache.ibatis.session.SqlSessionFactory;
import org.apache.ibatis.session.SqlSessionManager;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import javax.sql.DataSource;
import java.io.IOException;
import java.util.HashMap;
import java.util.Map;

@Configuration
public class MyBatisPlusConfig {

    /**
     * 注册动态表名插件
     */
    public static SqlSessionManager initSqlSessionFactory(DataSource dataSource) throws Exception {
        // 1. 创建MyBatis-Plus拦截器并添加动态表名拦截器
        MybatisPlusInterceptor interceptor = new MybatisPlusInterceptor();
        DynamicTableNameInnerInterceptor dynamicInterceptor = new DynamicTableNameInnerInterceptor();
        // 注册你的动态表名处理器（针对"can_message"表）
        dynamicInterceptor.setTableNameHandler(new DynamicTableNameHandler("can_message"));
        interceptor.addInnerInterceptor(dynamicInterceptor);

        // 2. 配置MyBatis
        MybatisConfiguration configuration = new MybatisConfiguration();
        configuration.addInterceptor(interceptor); // 关键：添加拦截器
        // 其他必要配置（如mapperLocations、typeAliasesPackage等）
        configuration.setMapUnderscoreToCamelCase(true); // 可选：下划线转驼峰

        // 3. 构建SqlSessionFactory
        MybatisSqlSessionFactoryBean sqlSessionFactoryBean = new MybatisSqlSessionFactoryBean();
        sqlSessionFactoryBean.setDataSource(dataSource);
        sqlSessionFactoryBean.setConfiguration(configuration);
        // 如果有mapper.xml文件，需要指定路径
        // sqlSessionFactoryBean.setMapperLocations(new PathMatchingResourcePatternResolver().getResources("classpath:mapper/*.xml"));

        SqlSessionFactory sqlSessionFactory = sqlSessionFactoryBean.getObject();
        return SqlSessionManager.newInstance(sqlSessionFactory);
    }
}
