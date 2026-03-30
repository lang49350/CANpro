package org.example.utils.web;

import org.example.pojo.data.CanMessage;

/**
 * 数据接收器通用接口
 * 支持UDP和USB两种通信方式
 * 
 * @author LiuHuan
 * @version 1.0
 */
public interface IDataReceiver {
    
    /**
     * 启动接收器
     */
    boolean start();
    
    /**
     * 停止接收器
     */
    void stop();
    
    /**
     * 检查是否已连接
     */
    boolean isConnected();
    
    /**
     * 发送CAN消息
     */
    boolean sendData(CanMessage message);
    
    /**
     * 获取项目ID
     */
    long getProjectId();
    
    /**
     * 获取连接状态字符串
     */
    String getConnectionStatus();
    
    /**
     * 重置统计信息
     */
    void resetStatistics();
}
