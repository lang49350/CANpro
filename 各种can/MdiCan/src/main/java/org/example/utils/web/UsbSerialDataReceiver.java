package org.example.utils.web;

import com.fazecast.jSerialComm.SerialPort;
import com.fazecast.jSerialComm.SerialPortDataListener;
import com.fazecast.jSerialComm.SerialPortEvent;
import org.example.pojo.data.CanMessage;
import org.example.ui.project.StandorTablePanel;
import org.example.ui.project.send.DoEventAction;
import org.example.utils.event.EventManager;
import org.example.utils.event.EventString;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.swing.*;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.*;
import java.util.concurrent.atomic.AtomicLong;

/**
 * USB串口CAN数据接收器
 * 用于与ESP32-S3 USB CDC通道通信
 * 支持FaCai协议 (0x1688) 和 CANX协议 (0x43414E58)
 * 
 * @author LiuHuan
 * @version 1.0 USB版本
 */
public class UsbSerialDataReceiver {
    private static final Logger logger = LoggerFactory.getLogger(UsbSerialDataReceiver.class);
    
    // ==================== USB串口配置 ====================
    private static final int DEFAULT_BAUD_RATE = 115200;
    private static final int DATA_BITS = 8;
    private static final int STOP_BITS = SerialPort.ONE_STOP_BIT;
    private static final int PARITY = SerialPort.NO_PARITY;
    private static final int READ_TIMEOUT = 100;  // 读取超时(ms)
    private static final int WRITE_TIMEOUT = 100; // 写入超时(ms)
    
    // ==================== 缓冲区配置 ====================
    private static final int BUFFER_SIZE = 65536;  // 64KB接收缓冲区
    private static final int QUEUE_SIZE = 500000;  // 50万条消息队列
    
    // ==================== 协议常量 ====================
    private static final int FACAI_MAGIC = 0x1688;      // FaCai协议魔数
    private static final int BINARY_MAGIC = 0x43414E58; // CANX协议魔数
    private static final int FACAI_HEADER_SIZE = 16;    // FaCai头部大小 (magic:2 + timestamp:8 + canId:4 + flags:1 + length:1)
    private static final int CANX_PACKET_SIZE = 84;     // CANX固定包大小
    
    // ==================== 串口对象 ====================
    private SerialPort serialPort;
    private String portName;
    private int baudRate = DEFAULT_BAUD_RATE;
    
    // ==================== 线程和队列 ====================
    private Thread processThread;
    private final ArrayBlockingQueue<byte[]> dataQueue = new ArrayBlockingQueue<>(QUEUE_SIZE);
    private final AtomicLong queueOverflowCount = new AtomicLong(0);
    private volatile boolean running = false;
    
    // ==================== 连接状态 ====================
    private volatile boolean connected = false;
    private volatile long lastDataTime = 0;
    private volatile long totalBytesReceived = 0;
    private volatile long totalPacketsReceived = 0;
    
    // ==================== UI和业务组件 ====================
    private StandorTablePanel mainFrame;
    private DataQueueMonitor queueMonitor;
    private DoEventAction doEventAction;
    private SaveMessageToDb saveMessageToDb = new SaveMessageToDb();
    
    // ==================== 数据缓冲 ====================
    private final ByteBuffer receiveBuffer = ByteBuffer.allocate(BUFFER_SIZE);
    private final Object bufferLock = new Object();

    // ==================== 构造函数 ====================
    
    public UsbSerialDataReceiver() {
        this.portName = null;
        receiveBuffer.order(ByteOrder.LITTLE_ENDIAN);
    }
    
    public UsbSerialDataReceiver(String portName) {
        this.portName = portName;
        receiveBuffer.order(ByteOrder.LITTLE_ENDIAN);
    }
    
    public UsbSerialDataReceiver(String portName, int baudRate) {
        this.portName = portName;
        this.baudRate = baudRate;
        receiveBuffer.order(ByteOrder.LITTLE_ENDIAN);
    }
    
    // ==================== 公共方法 ====================
    
    public void setMainFrame(StandorTablePanel mainFrame) {
        this.mainFrame = mainFrame;
    }
    
    public long getProjectId() {
        return mainFrame != null ? mainFrame.getProjectId() : 0;
    }
    
    /**
     * 获取所有可用的串口列表
     */
    public static String[] getAvailablePorts() {
        SerialPort[] ports = SerialPort.getCommPorts();
        String[] portNames = new String[ports.length];
        for (int i = 0; i < ports.length; i++) {
            portNames[i] = ports[i].getSystemPortName() + " - " + ports[i].getDescriptivePortName();
        }
        return portNames;
    }
    
    /**
     * 获取ESP32 USB CDC端口（自动检测）
     */
    public static String findESP32Port() {
        SerialPort[] ports = SerialPort.getCommPorts();
        for (SerialPort port : ports) {
            String desc = port.getDescriptivePortName().toLowerCase();
            // ESP32-S3 USB CDC通常包含这些关键字
            if (desc.contains("esp32") || desc.contains("usb") || 
                desc.contains("serial") || desc.contains("cp210") ||
                desc.contains("ch340") || desc.contains("ftdi")) {
                logger.info("检测到可能的ESP32端口: {} - {}", 
                           port.getSystemPortName(), port.getDescriptivePortName());
                return port.getSystemPortName();
            }
        }
        return null;
    }
    
    /**
     * 设置串口名称
     */
    public void setPortName(String portName) {
        this.portName = portName;
    }
    
    /**
     * 设置波特率
     */
    public void setBaudRate(int baudRate) {
        this.baudRate = baudRate;
    }
    
    /**
     * 启动USB串口数据接收
     */
    public boolean start() {
        if (running) {
            logger.warn("USB串口接收器已在运行中");
            return true;
        }
        
        try {
            // 自动检测端口
            if (portName == null || portName.isEmpty()) {
                portName = findESP32Port();
                if (portName == null) {
                    logger.error("未找到ESP32 USB端口，请手动指定");
                    JOptionPane.showMessageDialog(null, 
                        "未找到ESP32 USB端口，请检查连接或手动选择端口", 
                        "连接错误", JOptionPane.ERROR_MESSAGE);
                    return false;
                }
            }
            
            // 打开串口
            serialPort = SerialPort.getCommPort(portName);
            serialPort.setBaudRate(baudRate);
            serialPort.setNumDataBits(DATA_BITS);
            serialPort.setNumStopBits(STOP_BITS);
            serialPort.setParity(PARITY);
            serialPort.setComPortTimeouts(SerialPort.TIMEOUT_READ_SEMI_BLOCKING, 
                                          READ_TIMEOUT, WRITE_TIMEOUT);
            
            if (!serialPort.openPort()) {
                logger.error("无法打开串口: {}", portName);
                JOptionPane.showMessageDialog(null, 
                    "无法打开串口: " + portName + "\n请检查端口是否被占用", 
                    "连接错误", JOptionPane.ERROR_MESSAGE);
                return false;
            }
            
            running = true;
            connected = true;
            
            // 添加数据监听器
            serialPort.addDataListener(new SerialDataListener());
            
            // 启动数据处理线程
            processThread = new Thread(this::processDataFromQueue);
            processThread.setDaemon(true);
            processThread.setPriority(Thread.NORM_PRIORITY);
            processThread.setName("USB-Data-Processor");
            processThread.start();
            
            // 初始化业务组件
            doEventAction = new DoEventAction(null); // USB模式不需要UDP发送器
            queueMonitor = new DataQueueMonitor(dataQueue);
            queueMonitor.setProjectId(mainFrame != null ? mainFrame.getProjectId() : 0);
            queueMonitor.startMonitoring();
            
            // 发布连接事件
            EventManager.getInstance().notifyListeners(
                EventString.BOARD_CONNECT.name() + "#USB#" + portName
            );
            
            logger.info("✅ USB串口接收器已启动: {} @ {}bps", portName, baudRate);
            return true;
            
        } catch (Exception e) {
            logger.error("启动USB串口接收器失败", e);
            JOptionPane.showMessageDialog(null, 
                "启动USB串口接收器失败: " + e.getMessage(), 
                "错误", JOptionPane.ERROR_MESSAGE);
            return false;
        }
    }

    /**
     * 停止USB串口数据接收
     */
    public void stop() {
        if (!running) {
            return;
        }
        
        running = false;
        connected = false;
        
        // 停止处理线程
        if (processThread != null) {
            processThread.interrupt();
            try {
                processThread.join(1000);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
        }
        
        // 停止队列监控
        if (queueMonitor != null) {
            queueMonitor.stopMonitoring();
            queueMonitor = null;
        }
        
        // 关闭串口
        if (serialPort != null && serialPort.isOpen()) {
            serialPort.removeDataListener();
            serialPort.closePort();
            serialPort = null;
        }
        
        // 清空队列
        dataQueue.clear();
        
        // 保存未处理的数据
        saveMessageToDb.saveAll();
        saveMessageToDb.resetCounter();
        
        logger.info("USB串口接收器已停止");
    }
    
    /**
     * 检查是否已连接
     */
    public boolean isConnected() {
        return connected && serialPort != null && serialPort.isOpen();
    }
    
    /**
     * 获取连接状态字符串
     */
    public String getConnectionStatus() {
        if (!connected) {
            return "未连接";
        }
        return String.format("已连接: %s @ %dbps | 接收: %d包 / %.2fKB", 
                            portName, baudRate, totalPacketsReceived, 
                            totalBytesReceived / 1024.0);
    }
    
    // ==================== 串口数据监听器 ====================
    
    private class SerialDataListener implements SerialPortDataListener {
        @Override
        public int getListeningEvents() {
            return SerialPort.LISTENING_EVENT_DATA_AVAILABLE;
        }
        
        @Override
        public void serialEvent(SerialPortEvent event) {
            if (event.getEventType() != SerialPort.LISTENING_EVENT_DATA_AVAILABLE) {
                return;
            }
            
            try {
                int available = serialPort.bytesAvailable();
                if (available <= 0) {
                    return;
                }
                
                byte[] buffer = new byte[available];
                int bytesRead = serialPort.readBytes(buffer, available);
                
                if (bytesRead > 0) {
                    totalBytesReceived += bytesRead;
                    lastDataTime = System.currentTimeMillis();
                    
                    // 记录接收数据大小
                    if (mainFrame != null) {
                        ReceivedDataReCorder.getInstance()
                            .addProjectReceivedDataCount(mainFrame.getProjectId(), bytesRead);
                    }
                    
                    // 将数据放入处理队列
                    synchronized (bufferLock) {
                        processIncomingData(buffer, bytesRead);
                    }
                }
            } catch (Exception e) {
                logger.error("串口数据接收错误", e);
            }
        }
    }
    
    /**
     * 处理接收到的原始数据，提取完整的数据包
     */
    private void processIncomingData(byte[] data, int length) {
        receiveBuffer.put(data, 0, length);
        receiveBuffer.flip();
        
        while (receiveBuffer.remaining() >= 2) {
            receiveBuffer.mark();
            
            // 检查魔数
            int magic = receiveBuffer.getShort() & 0xFFFF;
            receiveBuffer.reset();
            
            if (magic == FACAI_MAGIC) {
                // FaCai协议
                if (receiveBuffer.remaining() < FACAI_HEADER_SIZE) {
                    break; // 数据不完整，等待更多数据
                }
                
                // 保存当前位置
                int startPos = receiveBuffer.position();
                
                // 跳过魔数(2) + 时间戳(8) + canId(4) + flags(1) = 15字节，读取数据长度
                receiveBuffer.position(startPos + 15);
                int dataLen = receiveBuffer.get() & 0xFF;
                receiveBuffer.position(startPos); // 恢复位置
                
                // 验证数据长度
                if (dataLen > 64) {
                    logger.warn("FaCai数据长度异常: {}, 跳过此包", dataLen);
                    receiveBuffer.get(); // 跳过一个字节继续搜索
                    continue;
                }
                
                int packetSize = FACAI_HEADER_SIZE + dataLen;
                if (receiveBuffer.remaining() < packetSize) {
                    break; // 数据不完整，等待更多数据
                }
                
                // 提取完整包
                byte[] packet = new byte[packetSize];
                receiveBuffer.get(packet);
                enqueuePacket(packet);
                
            } else if (receiveBuffer.remaining() >= 4) {
                // 检查CANX协议
                int magic32 = receiveBuffer.getInt();
                receiveBuffer.reset();
                
                if (magic32 == BINARY_MAGIC) {
                    if (receiveBuffer.remaining() < CANX_PACKET_SIZE) {
                        break; // 数据不完整
                    }
                    
                    byte[] packet = new byte[CANX_PACKET_SIZE];
                    receiveBuffer.get(packet);
                    enqueuePacket(packet);
                } else {
                    // 未知数据，跳过一个字节继续搜索
                    receiveBuffer.get();
                }
            } else {
                break;
            }
        }
        
        // 压缩缓冲区
        receiveBuffer.compact();
    }
    
    /**
     * 将数据包放入处理队列
     */
    private void enqueuePacket(byte[] packet) {
        totalPacketsReceived++;
        
        if (!dataQueue.offer(packet)) {
            // 队列满，清理旧数据
            int clearCount = Math.min(10000, dataQueue.size() / 5);
            for (int i = 0; i < clearCount; i++) {
                dataQueue.poll();
            }
            
            if (!dataQueue.offer(packet)) {
                queueOverflowCount.incrementAndGet();
                if (queueOverflowCount.get() % 5000 == 0) {
                    logger.warn("数据队列溢出: {} 次", queueOverflowCount.get());
                }
            }
        }
        
        // 更新队列监控
        if (queueMonitor != null) {
            queueMonitor.setQueueActived(true);
            queueMonitor.setLastDataTime(System.currentTimeMillis());
            queueMonitor.resetTriggered();
        }
    }

    // ==================== 数据处理线程 ====================
    
    /**
     * 数据处理线程 - 负责解析数据和UI更新
     */
    private void processDataFromQueue() {
        logger.info("🚀 USB数据处理线程启动");
        
        while (running) {
            try {
                List<byte[]> batchData = new ArrayList<>();
                
                // 等待数据
                byte[] firstData = dataQueue.poll(20, TimeUnit.MILLISECONDS);
                if (firstData != null) {
                    batchData.add(firstData);
                    
                    // 批量取数据
                    for (int i = 1; i < 50000; i++) {
                        byte[] moreData = dataQueue.poll();
                        if (moreData == null) break;
                        batchData.add(moreData);
                    }
                    
                    // 处理数据
                    long startTime = System.nanoTime();
                    int queueSize = dataQueue.size();
                    boolean highLoad = queueSize > 1000 || batchData.size() > 1000;
                    
                    for (byte[] data : batchData) {
                        try {
                            processReceivedData(data);
                        } catch (Exception e) {
                            if (!highLoad) {
                                logger.warn("数据处理错误: {}", e.getMessage());
                            }
                        }
                    }
                    
                    long processTime = (System.nanoTime() - startTime) / 1_000_000;
                    
                    if (batchData.size() > 100) {
                        logger.info("批量处理: {}条, 耗时: {}ms, 队列: {}", 
                                   batchData.size(), processTime, queueSize);
                    }
                }
                
            } catch (InterruptedException e) {
                logger.info("数据处理线程被中断");
                break;
            } catch (Exception e) {
                logger.error("数据处理出错", e);
                try {
                    Thread.sleep(10);
                } catch (InterruptedException ie) {
                    break;
                }
            }
        }
    }
    
    /**
     * 处理接收到的数据包
     */
    private void processReceivedData(byte[] data) {
        if (data.length >= FACAI_HEADER_SIZE && isFaCaiProtocol(data)) {
            parseFaCaiPacket(data);
        } else if (data.length == CANX_PACKET_SIZE && isBinaryProtocol(data)) {
            parseCANXPacket(data);
        } else {
            logger.debug("未知协议数据: 长度={}", data.length);
        }
    }
    
    /**
     * 检测是否为FaCai协议
     */
    private boolean isFaCaiProtocol(byte[] data) {
        if (data.length < FACAI_HEADER_SIZE) return false;
        int magic = ByteBuffer.wrap(data, 0, 2)
                             .order(ByteOrder.LITTLE_ENDIAN)
                             .getShort() & 0xFFFF;
        return magic == FACAI_MAGIC;
    }
    
    /**
     * 检测是否为CANX协议
     */
    private boolean isBinaryProtocol(byte[] data) {
        if (data.length < 4) return false;
        int magic = ByteBuffer.wrap(data, 0, 4)
                             .order(ByteOrder.LITTLE_ENDIAN)
                             .getInt();
        return magic == BINARY_MAGIC;
    }
    
    /**
     * 解析FaCai协议数据包
     */
    private void parseFaCaiPacket(byte[] data) {
        try {
            // 先检查最小长度
            if (data == null || data.length < FACAI_HEADER_SIZE) {
                logger.warn("FaCai数据包太短: 长度={}", data == null ? 0 : data.length);
                return;
            }
            
            ByteBuffer buffer = ByteBuffer.wrap(data);
            buffer.order(ByteOrder.LITTLE_ENDIAN);
            
            // 解析头部
            int magic = buffer.getShort() & 0xFFFF;
            if (magic != FACAI_MAGIC) {
                logger.warn("FaCai魔数验证失败: 0x{}", Integer.toHexString(magic));
                return;
            }
            
            // 🚀 正确解析：64位相对时间戳（微秒，从首条CAN消息开始）
            long relativeTimestampUs = buffer.getLong();  // 纯64位相对时间（微秒）
            
            // 转换为毫秒用于显示（保留6位小数精度）
            double relativeTimestampMs = relativeTimestampUs / 1000.0;
            
            long canId = Integer.toUnsignedLong(buffer.getInt());
            int flags = buffer.get() & 0xFF;
            int len = buffer.get() & 0xFF;
            
            // 验证数据长度
            if (len > 64) {
                logger.warn("FaCai数据长度异常: {}, 截断为64", len);
                len = 64;
            }
            
            // 检查实际数据是否足够
            int expectedSize = FACAI_HEADER_SIZE + len;
            if (data.length < expectedSize) {
                logger.warn("FaCai包长度不足: 期望={}, 实际={}", expectedSize, data.length);
                return;
            }
            
            // 检查buffer剩余空间
            if (buffer.remaining() < len) {
                logger.warn("FaCai buffer剩余不足: 需要={}, 剩余={}", len, buffer.remaining());
                return;
            }
            
            byte[] canData = new byte[len];
            if (len > 0) {
                buffer.get(canData, 0, len);
            }
            
            // 解析标志位
            boolean isExtended = (flags & 0x01) != 0;
            boolean isRTR = (flags & 0x02) != 0;
            boolean isCANFD = (flags & 0x04) != 0;
            
            // 创建CAN消息
            CanMessage canMsg = new CanMessage();
            canMsg.setCanId(String.format(isExtended ? "%08X" : "%03X", canId));
            canMsg.setData(bytesToHexString(canData));
            
            // 🚀 使用相对时间戳（CANoe风格：秒.微秒）
            canMsg.setTime(formatRelativeTime(relativeTimestampMs));
            
            canMsg.setDirection(isRTR ? "RTR" : "接收");
            canMsg.setIndexCounter(relativeTimestampUs);  // 使用相对时间作为索引
            canMsg.setProjectId(mainFrame != null ? mainFrame.getProjectId() : 0);
            
            addCANMessageToQueue(canMsg);
            
            logger.debug("FaCai消息: ID={}, 相对时间={}ms, 长度={}", 
                        canMsg.getCanId(), String.format("%.6f", relativeTimestampMs), len);
            
        } catch (Exception e) {
            logger.error("FaCai解析失败: 数据长度={}", data != null ? data.length : 0, e);
        }
    }
    
    /**
     * 解析CANX协议数据包
     */
    private void parseCANXPacket(byte[] data) {
        try {
            ByteBuffer buffer = ByteBuffer.wrap(data);
            buffer.order(ByteOrder.LITTLE_ENDIAN);
            
            int magic = buffer.getInt();
            if (magic != BINARY_MAGIC) {
                logger.warn("CANX魔数验证失败: 0x{}", Integer.toHexString(magic));
                return;
            }
            
            long timestampWithIndex = buffer.getLong();
            long esp32Index = timestampWithIndex & 0xFFFFFFFFL;
            long actualTimestamp = System.currentTimeMillis();
            
            long canId = Integer.toUnsignedLong(buffer.getInt());
            int len = buffer.get() & 0xFF;
            int flags = buffer.get() & 0xFF;
            buffer.getShort(); // 跳过保留字节
            
            if (len > 64) len = 64;
            
            byte[] canData = new byte[len];
            buffer.get(canData, 0, len);
            
            boolean isExtended = (flags & 0x01) != 0;
            
            CanMessage canMsg = new CanMessage();
            canMsg.setCanId(String.format(isExtended ? "%08X" : "%03X", canId));
            canMsg.setData(bytesToHexString(canData));
            canMsg.setTime(formatTimestamp(actualTimestamp));
            canMsg.setDirection("接收");
            canMsg.setIndexCounter(esp32Index);
            canMsg.setProjectId(mainFrame != null ? mainFrame.getProjectId() : 0);
            
            addCANMessageToQueue(canMsg);
            
        } catch (Exception e) {
            logger.error("CANX解析失败", e);
        }
    }

    // ==================== 发送功能 ====================
    
    /**
     * 发送CAN消息到ESP32
     */
    public boolean sendData(CanMessage message) {
        if (!isConnected()) {
            logger.warn("USB未连接，无法发送数据");
            return false;
        }
        
        try {
            // 优先使用FaCai协议
            byte[] packet = createFaCaiPacket(message);
            int written = serialPort.writeBytes(packet, packet.length);
            
            if (written == packet.length) {
                logger.debug("发送FaCai包: ID={}, 大小={}字节", 
                            message.getCanId(), packet.length);
                return true;
            } else {
                logger.warn("发送不完整: 期望={}, 实际={}", packet.length, written);
                return false;
            }
            
        } catch (Exception e) {
            logger.error("发送数据失败", e);
            return false;
        }
    }
    
    /**
     * 发送命令到ESP32
     */
    public boolean sendCommand(String command) {
        if (!isConnected()) {
            logger.warn("USB未连接，无法发送命令");
            return false;
        }
        
        try {
            byte[] data = (command + "\n").getBytes();
            int written = serialPort.writeBytes(data, data.length);
            return written == data.length;
        } catch (Exception e) {
            logger.error("发送命令失败", e);
            return false;
        }
    }
    
    /**
     * 发送时间同步请求
     */
    public boolean sendTimeSync() {
        long timestamp = System.currentTimeMillis() * 1000; // 转换为微秒
        return sendCommand("TIME_SYNC:" + timestamp);
    }
    
    /**
     * 创建FaCai协议数据包
     */
    private byte[] createFaCaiPacket(CanMessage message) {
        byte[] canData = hexStringToBytes(message.getData());
        int dataLength = Math.min(canData.length, 64);
        
        ByteBuffer buffer = ByteBuffer.allocate(FACAI_HEADER_SIZE + dataLength);
        buffer.order(ByteOrder.LITTLE_ENDIAN);
        
        // 魔数
        buffer.putShort((short) FACAI_MAGIC);
        
        // 时间戳
        long timestamp = System.currentTimeMillis() * 1000;
        buffer.putLong(timestamp);
        
        // CAN ID
        long canId = parseCANId(message.getCanId());
        buffer.putInt((int) canId);
        
        // 标志位
        byte flags = 0;
        if (canId > 0x7FF) flags |= 0x01;
        if ("RTR".equals(message.getDirection())) flags |= 0x02;
        if (dataLength > 8) flags |= 0x04;
        buffer.put(flags);
        
        // 数据长度
        buffer.put((byte) dataLength);
        
        // 数据
        buffer.put(canData, 0, dataLength);
        
        return buffer.array();
    }
    
    // ==================== 工具方法 ====================
    
    /**
     * 添加CAN消息到UI队列
     */
    private void addCANMessageToQueue(CanMessage message) {
        if (mainFrame != null) {
            mainFrame.addCanMessage(message);
        }
        // 使用save方法批量保存
        if (message.getProjectId() != null) {
            java.util.List<CanMessage> messages = new java.util.ArrayList<>();
            messages.add(message);
            saveMessageToDb.save(messages, message.getProjectId());
        }
    }
    
    /**
     * 字节数组转十六进制字符串
     */
    private String bytesToHexString(byte[] bytes) {
        StringBuilder sb = new StringBuilder();
        for (byte b : bytes) {
            sb.append(String.format("%02X", b));
        }
        return sb.toString();
    }
    
    /**
     * 十六进制字符串转字节数组
     */
    private byte[] hexStringToBytes(String hex) {
        if (hex == null || hex.isEmpty()) {
            return new byte[0];
        }
        hex = hex.replaceAll("\\s+", "");
        int len = hex.length();
        byte[] data = new byte[len / 2];
        for (int i = 0; i < len; i += 2) {
            data[i / 2] = (byte) ((Character.digit(hex.charAt(i), 16) << 4)
                                 + Character.digit(hex.charAt(i + 1), 16));
        }
        return data;
    }
    
    /**
     * 解析CAN ID
     */
    private long parseCANId(String canIdStr) {
        try {
            return Long.parseLong(canIdStr.replaceAll("[^0-9A-Fa-f]", ""), 16);
        } catch (NumberFormatException e) {
            return 0;
        }
    }
    
    /**
     * 格式化时间戳（CANoe风格）
     */
    private String formatTimestamp(long millis) {
        double seconds = millis / 1000.0;
        return String.format("%.6f", seconds % 100000);
    }
    
    /**
     * 格式化相对时间戳（CANoe风格：秒.微秒）
     * @param milliseconds 相对时间（毫秒）
     * @return 格式化字符串，例如 "0.001234" 表示1.234毫秒
     */
    private String formatRelativeTime(double milliseconds) {
        double seconds = milliseconds / 1000.0;
        return String.format("%.6f", seconds);
    }
    
    // ==================== 统计信息 ====================
    
    public long getTotalBytesReceived() {
        return totalBytesReceived;
    }
    
    public long getTotalPacketsReceived() {
        return totalPacketsReceived;
    }
    
    public long getQueueOverflowCount() {
        return queueOverflowCount.get();
    }
    
    public int getQueueSize() {
        return dataQueue.size();
    }
    
    public long getLastDataTime() {
        return lastDataTime;
    }
    
    public String getPortName() {
        return portName;
    }
    
    public int getBaudRate() {
        return baudRate;
    }
    
    /**
     * 重置统计信息
     */
    public void resetStatistics() {
        totalBytesReceived = 0;
        totalPacketsReceived = 0;
        queueOverflowCount.set(0);
        saveMessageToDb.resetCounter();
        logger.info("统计信息已重置");
    }
}
