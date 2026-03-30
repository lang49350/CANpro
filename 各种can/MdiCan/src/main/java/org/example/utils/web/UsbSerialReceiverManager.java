package org.example.utils.web;

import com.fazecast.jSerialComm.SerialPort;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

/**
 * USB串口接收器管理器
 * 单例模式，管理所有USB串口连接
 * 
 * @author LiuHuan
 * @version 1.0
 */
public class UsbSerialReceiverManager {
    private static final Logger logger = LoggerFactory.getLogger(UsbSerialReceiverManager.class);
    
    private static UsbSerialReceiverManager instance;
    
    // 使用端口名作为key
    private final Map<String, UsbSerialDataReceiver> receivers = new ConcurrentHashMap<>();
    
    private UsbSerialReceiverManager() {
    }
    
    public static synchronized UsbSerialReceiverManager getInstance() {
        if (instance == null) {
            instance = new UsbSerialReceiverManager();
        }
        return instance;
    }
    
    /**
     * 添加接收器
     */
    public void addReceiver(String portName, UsbSerialDataReceiver receiver) {
        receivers.put(portName, receiver);
        logger.info("添加USB接收器: {}", portName);
    }
    
    /**
     * 获取接收器
     */
    public UsbSerialDataReceiver getReceiver(String portName) {
        return receivers.get(portName);
    }
    
    /**
     * 移除接收器
     */
    public void removeReceiver(String portName) {
        UsbSerialDataReceiver receiver = receivers.remove(portName);
        if (receiver != null) {
            receiver.stop();
            logger.info("移除USB接收器: {}", portName);
        }
    }
    
    /**
     * 检查是否存在接收器
     */
    public boolean hasReceiver(String portName) {
        return receivers.containsKey(portName);
    }
    
    /**
     * 获取第一个可用的接收器
     */
    public UsbSerialDataReceiver getFirstReceiver() {
        if (receivers.isEmpty()) {
            return null;
        }
        return receivers.values().iterator().next();
    }
    
    /**
     * 停止所有接收器
     */
    public void stopAllReceivers() {
        for (UsbSerialDataReceiver receiver : receivers.values()) {
            receiver.stop();
        }
        logger.info("已停止所有USB接收器");
    }
    
    /**
     * 移除所有接收器
     */
    public void removeAllReceivers() {
        stopAllReceivers();
        receivers.clear();
        logger.info("已移除所有USB接收器");
    }
    
    /**
     * 获取所有接收器
     */
    public Map<String, UsbSerialDataReceiver> getAllReceivers() {
        return new HashMap<>(receivers);
    }
    
    /**
     * 获取接收器数量
     */
    public int getReceiverCount() {
        return receivers.size();
    }
    
    /**
     * 获取所有可用的串口列表
     */
    public static String[] getAvailablePorts() {
        return UsbSerialDataReceiver.getAvailablePorts();
    }
    
    /**
     * 自动检测并连接ESP32
     */
    public UsbSerialDataReceiver autoConnectESP32() {
        String portName = UsbSerialDataReceiver.findESP32Port();
        if (portName == null) {
            logger.warn("未找到ESP32 USB端口");
            return null;
        }
        
        if (hasReceiver(portName)) {
            logger.info("ESP32已连接: {}", portName);
            return getReceiver(portName);
        }
        
        UsbSerialDataReceiver receiver = new UsbSerialDataReceiver(portName);
        if (receiver.start()) {
            addReceiver(portName, receiver);
            return receiver;
        }
        
        return null;
    }
    
    /**
     * 刷新可用端口列表
     */
    public SerialPort[] refreshPorts() {
        return SerialPort.getCommPorts();
    }
    
    /**
     * 获取连接状态摘要
     */
    public String getStatusSummary() {
        StringBuilder sb = new StringBuilder();
        sb.append("USB串口连接状态:\n");
        
        if (receivers.isEmpty()) {
            sb.append("  无活动连接\n");
        } else {
            for (Map.Entry<String, UsbSerialDataReceiver> entry : receivers.entrySet()) {
                UsbSerialDataReceiver receiver = entry.getValue();
                sb.append(String.format("  %s: %s\n", 
                    entry.getKey(), 
                    receiver.isConnected() ? "已连接" : "已断开"));
            }
        }
        
        return sb.toString();
    }
}
