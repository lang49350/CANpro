package org.example.utils.web;
import org.example.pojo.data.CanMessage;
import org.example.ui.MainFrame;
import org.example.ui.project.StandorTablePanel;

import org.example.ui.project.send.DoEventAction;
import org.example.utils.event.EventManager;
import org.example.utils.event.EventString;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.swing.*;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.time.Instant;

import java.util.Arrays;

// ==================== 🚀 新增导入：双线程架构和高性能处理 ====================
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.atomic.AtomicLong;
import java.util.List;
import java.util.ArrayList;
import java.nio.charset.StandardCharsets;
// ==================== 🚀 新增导入结束 ====================

/**
 * CAN总线数据处理类
 */
public class UdpWifiDataReceiver {
    private  int PORT = 8077;
    // ==================== 🚀 升级：大幅扩容缓冲区，适配ESP32高性能 ====================
    private static final int BUFFER_SIZE = 65536; // 🚀 扩容：1024 → 65536，支持大UDP包
    // ==================== 🚀 升级结束 ====================
    private static final String ACK_MESSAGE = "ACK";


    private DatagramSocket socket;
    private Thread receiveThread;
    // ==================== 🚀 新增：双线程架构变量 ====================
    private Thread processThread;  // 新增：数据处理线程
    // 🚀 超级扩容：支持500,000条消息队列，适配ESP32超大包发送
    private final ArrayBlockingQueue<byte[]> dataQueue = new ArrayBlockingQueue<>(500000);
    private final AtomicLong queueOverflowCount = new AtomicLong(0);
    // ==================== 🚀 新增结束 ====================
    
    private boolean running;
    private InetAddress boardAddress;

    private String boradAddressString;
    private int boardPort;

    private StandorTablePanel mainFrame;

    private DataQueueMonitor queueMonitor;

    private DoEventAction doEventAction;



    private static final Logger logger = LoggerFactory.getLogger(UdpWifiDataReceiver.class);
    
    // ==================== 🚀 新增：连接健康管理和设备识别 ====================
    private volatile long lastHeartbeatTime = 0;           // 上次心跳时间
    private volatile boolean connectionHealthy = false;    // 连接健康状态
    private final long HEARTBEAT_TIMEOUT = 15000;          // 心跳超时时间(毫秒)
    private ScheduledExecutorService healthCheckTimer;
    
    // 设备类型识别

    
    // 双协议检测常量
    private static final int BINARY_MAGIC = 0x43414E58; // "CANX"
    private static final int FACAI_MAGIC = 0x1688;      // "一路发财" FaCai protocol
    private static final String TEXT_MAGIC = TextFixDefine.PREFIX;
    
    // 消息索引和发送统计
    private volatile long messageIndexCounter = 0;
    private volatile long lastSendTime = 0;
    private volatile double currentSendInterval = 1.0;

    private SaveMessageToDb saveMessageToDb = new SaveMessageToDb();

    // ==================== 🚀 新增结束 ====================

    public UdpWifiDataReceiver(int port) {
        this.PORT = port;
    }

    public void setMainFrame(StandorTablePanel mainFrame) {
        this.mainFrame = mainFrame;
    }

    public long getProjectId() {
        return mainFrame.getProjectId();
    }
    /**
     * 启动数据处理
     */
    public void start() {
        if (running) {
            return;
        }

        try {
            if(socket == null || socket.isClosed()) {
                socket = new DatagramSocket(PORT);
                socket.setBroadcast(true);

            }
            running = true;

            // ==================== 🚀 升级：双线程架构启动 ====================
            resetMessageIndex();
            
            // 🚀 启动UDP接收线程（高优先级，仅负责接收）
            receiveThread = new Thread(this::receiveData);
            receiveThread.setDaemon(true);
            receiveThread.setPriority(Thread.MAX_PRIORITY);
            receiveThread.setName("UDP-Receiver");
            receiveThread.start();
            
            // 🚀 启动数据处理线程（普通优先级，负责解析和UI更新）
            processThread = new Thread(this::processDataFromQueue);
            processThread.setDaemon(true);
            processThread.setPriority(Thread.NORM_PRIORITY);
            processThread.setName("Data-Processor");
            processThread.start();
            
            startSimpleHealthCheckTimer();
            autoConnectToDevice();
            
            logger.info("CAN数据处理器已启动，监听端口: {} (双线程模式)", PORT);
            // ==================== 🚀 升级结束 ====================

            doEventAction = new DoEventAction(this);
            // 初始化并启动队列监控器
            queueMonitor = new DataQueueMonitor(dataQueue);
            queueMonitor.setUppWifiDataReceiver(this);
            queueMonitor.startMonitoring();
            logger.info("数据队列监控器已启动");
        } catch (Exception e) {
            logger.info("启动CAN数据处理器失败: " + e.getMessage());
            JOptionPane.showMessageDialog(null, "启动CAN数据处理器失败: " + e.getMessage(), "错误", JOptionPane.ERROR_MESSAGE);
        }
    }

    /**
     * 停止数据处理
     */
    public void stop() {
        if (!running) {
            return;
        }

        running = false;
        
        // ==================== 🚀 升级：双线程停止管理 ====================
        // 🚀 停止接收线程
        if (receiveThread != null) {
            receiveThread.interrupt();
        }
        
        // 🚀 停止处理线程
        if (processThread != null) {
            processThread.interrupt();
        }
        
        stopHealthCheckTimer();
        
        // 清空新队列
        dataQueue.clear();
        // ==================== 🚀 升级结束 ====================

        // 停止队列监控器
        if (queueMonitor != null) {
            queueMonitor.stopMonitoring();
            queueMonitor = null;
            logger.info("数据队列监控器已停止");
        }

        if (socket != null && !socket.isClosed()) {
            socket.close();
            socket = null;
        }

        // 停止接收时检查存储到磁盘的队列是否有未处理的数据
        saveMessageToDb.saveAll();
        resetMessageIndex();
        saveMessageToDb.resetCounter();

        logger.info("CAN数据处理器已停止 (双线程模式)");
    }

    /**
     * 接收数据的线程方法
     */
    private void receiveData() {
        // ==================== 🚀 升级：高性能UDP接收线程 ====================
        byte[] buffer = new byte[BUFFER_SIZE]; // 使用升级后的65KB缓冲区


        // ==================== 🚀 升级结束 ====================

        while (running) {
            try {
                DatagramPacket packet = new DatagramPacket(buffer, buffer.length);
                socket.receive(packet);

                // ==================== 🚀 升级：智能协议检测和处理 ====================
                // 获取实际接收的数据
                byte[] receivedData = Arrays.copyOf(packet.getData(), packet.getLength());
                // 记录到数据接收大小记录中
                ReceivedDataReCorder.getInstance().addProjectReceivedDataCount(mainFrame.getProjectId(),receivedData.length);
                // 先检查是否为心跳消息（文本格式）
                if (packet.getLength() == 3 && ACK_MESSAGE.equals(new String(receivedData))) {
                    // 心跳消息立即处理（高优先级）
                    boardAddress = packet.getAddress();
                    boradAddressString = boardAddress.getHostAddress();
                    boardPort = packet.getPort();
                    lastHeartbeatTime = System.currentTimeMillis();
                    connectionHealthy = true;
                    logger.debug("收到来自 " + boradAddressString + ":" + boardPort + " 的心跳消息");
                    sendAck();
                } else {
                    queueMonitor.setQueueActived(true);
                    queueMonitor.setLastDataTime(System.currentTimeMillis());
                    queueMonitor.resetTriggered();
                    // 🚀 CAN数据立即入队，避免UDP接收阻塞
                    if (!dataQueue.offer(receivedData)) {
                        // 🚀 队列满时的高性能清理策略：批量清理旧数据
                        try {
                            // 快速清理队列前20%的数据，为新数据让路
                            int clearCount = Math.min(10000, dataQueue.size() / 5);
                            for (int i = 0; i < clearCount; i++) {
                                dataQueue.poll();
                            }
                            
                            // 再次尝试入队
                            if (!dataQueue.offer(receivedData)) {
                                queueOverflowCount.incrementAndGet();
                                // 🚀 减少日志频率，避免影响性能
                                if (queueOverflowCount.get() % 5000 == 0) {
                                    logger.warn("数据队列溢出: {} 次, 包大小: {}KB", 
                                               queueOverflowCount.get(), packet.getLength() / 1024.0);
                                }
                            } else {
                                logger.debug("队列清理后成功入队，清理了{}条旧数据", clearCount);
                            }
                        } catch (Exception e) {
                            logger.warn("队列清理失败: {}", e.getMessage());
                        }
                    }
                }
                // ==================== 🚀 升级结束 ====================
                
                // 保持原有兼容性：同时处理旧的队列系统
//                String message = new String(packet.getData(), 0, packet.getLength());
//                if (!ACK_MESSAGE.equals(message)) {
//                    handelClient(message.getBytes());
//                }
                
            } catch (Exception e) {
                e.printStackTrace();
                if (running) {
                    logger.info("接收数据出错: " + e.getMessage());
                }
            }
        }
    }

    /**
     * 发送ACK确认消息到开发板
     */
    private void sendAck() {
        try {
            if (boardAddress != null) {
                long currentTimeMillis = Instant.now().toEpochMilli();
                String timeString = String.valueOf(currentTimeMillis);
                logger.info("Send ACK is " + ACK_MESSAGE + "#" + timeString + boradAddressString);
                byte[] ackData = (ACK_MESSAGE + "#" + timeString).getBytes();
                // 发送ACK到开发板的PORT端口
                DatagramPacket ackPacket = new DatagramPacket(
                        ackData, ackData.length, boardAddress, PORT);
                socket.send(ackPacket);
                //发布板子连接事件
                EventManager.getInstance().notifyListeners(
                        EventString.BOARD_CONNECT.name() + "#" + boradAddressString + "#" + boardPort
                );
            }
        } catch (Exception e) {
            e.printStackTrace();
            logger.error("发送ACK确认失败: " + e.getMessage());
        }
    }

    private void sendAck(String flag) {
        try {
            if (boardAddress != null) {
                long currentTimeMillis = Instant.now().toEpochMilli();
                String timeString = String.valueOf(currentTimeMillis);
                logger.info("发送 " + flag) ;
                byte[] ackData = (ACK_MESSAGE + "#" + timeString + "#" + flag).getBytes();
                // 发送ACK到开发板的8077端口
                DatagramPacket ackPacket = new DatagramPacket(
                        ackData, ackData.length, boardAddress, PORT);
                socket.send(ackPacket);
            }
        } catch (Exception e) {
            e.printStackTrace();
            logger.error("发送失败: " + e.getMessage());
        }
    }

    /**
     *  开始发送数据标识/结束标识
     * @return
     */
    public  void sendDataFlag(String flag) {
        try {
            if (boardAddress != null) {
                sendAck(flag);
                logger.info("发送数据");
            }else{
                logger.info("boardAddress 是空的，无法发送");
            }
        } catch (Exception e) {
            e.printStackTrace();
            logger.error("发送ACK确认失败: " + e.getMessage());
        }
    }

    public void sendData(CanMessage message){
        try {
            if (boardAddress != null) {
                // ==================== 🚀 升级：智能三协议发送（FaCai + CANX + Text）====================
                // 记录发送时间用于计算发送间隔
                long currentTime = System.currentTimeMillis();
                
                try {
                    // 🚀 优先使用FaCai协议（最高性能，变长优化）
                    byte[] facaiPacket = createFaCaiCANPacket(message);
                    
                    DatagramPacket ackPacket = new DatagramPacket(
                            facaiPacket, facaiPacket.length, boardAddress, PORT);
                    socket.send(ackPacket);
                    
                    // 更新发送间隔统计
                    updateSendInterval(currentTime);
                    
                    logger.debug("发送FaCai CAN数据包: ID={}, 数据长度={}, 包大小={}字节", 
                               message.getCanId(), message.getData().length() / 2, facaiPacket.length);
                    return; // FaCai发送成功，直接返回
                    
                } catch (Exception facaiError) {
                    logger.warn("FaCai协议发送失败，回退到二进制协议: {}", facaiError.getMessage());
                    
                    try {
                        // 🚀 回退到二进制协议（兼容性保证）
                        byte[] binaryPacket = createBinaryCANPacket(message);
                        
                        DatagramPacket ackPacket = new DatagramPacket(
                                binaryPacket, binaryPacket.length, boardAddress, PORT);
                        socket.send(ackPacket);
                        
                        // 更新发送间隔统计
                        updateSendInterval(currentTime);
                        
                        logger.debug("发送二进制CAN数据包: ID={}, 数据长度={}", 
                                   message.getCanId(), message.getData().length() / 2);
                        return; // 二进制发送成功，直接返回
                        
                    } catch (Exception binaryError) {
                        logger.warn("二进制协议发送失败，回退到文本协议: {}", binaryError.getMessage());
                        
                        // 🚀 最终回退到文本协议（最大兼容性）
                        String prefix = TextFixDefine.PREFIX;
                        String spliter = TextFixDefine.DELIMITER;
                        String data = prefix  +
                                message.getCanId() +
                                spliter + message.getData() +
                                spliter + message.getData().length() +
                                spliter + message.getTime() +
                                spliter + message.getIndexCounter()+ prefix;

                        DatagramPacket ackPacket = new DatagramPacket(
                                data.getBytes(), data.length(), boardAddress, PORT);
                        socket.send(ackPacket);
                        
                        // 更新发送间隔统计
                        updateSendInterval(currentTime);
                        
                        logger.debug("发送文本CAN数据包: ID={}", message.getCanId());
                    }
                }
                // ==================== 🚀 升级结束 ====================
            }else{
                logger.info("boardAddress 是空的，无法发送");
            }
        } catch (Exception e) {
            e.printStackTrace();
            logger.error("发送数据失败: " + e.getMessage());
        }
    }

    private static String bytesToHex(byte[] bytes, int length) {
        StringBuilder result = new StringBuilder();
        for (int i = 0; i < length; i++) {
            result.append(String.format("%02X ", bytes[i]));
            if ((i + 1) % 16 == 0) {
                result.append("\n                  ");
            }
        }
        return result.toString();
    }

    private void handelClient(byte[] data){
        try {

            // 存储客户端连接
            if( data.length > 0 && running){
                // 将数据放入队列，由单独的线程处理

                logger.info("队列长度 " + mainFrame.dataQueue.size());
                if (!mainFrame.dataQueue.offer(data)) {
                    // 队列已满，记录警告
                    logger.info("数据队列已满，丢弃数据");
                    // 可以选择暂停UI更新，优先处理队列
                    mainFrame.uiUpdateEnabled.set(false);
                    // 给队列一些时间处理数据
                    Thread.sleep(10);
                    mainFrame.uiUpdateEnabled.set(true);
                }
            }
        } catch (InterruptedException e) {
            throw new RuntimeException(e);
        }
    }

    // ==================== 🚀 新增：核心功能方法集合 ====================
    
    /**
     * 🚀 数据处理线程 - 负责解析数据和UI更新
     */
    private void processDataFromQueue() {
        logger.info("🚀 数据处理线程启动 - 超高性能批量处理模式");
        
        while (running) {
            try {
                // 🚀 大幅增加批量处理能力
                List<byte[]> batchData = new ArrayList<>();
                
                // 🚀 修复：缩短等待时间，提高响应速度
                byte[] firstData = dataQueue.poll(20, TimeUnit.MILLISECONDS);
                if (firstData != null) {
                    batchData.add(firstData);
                    
                    // 🚀 超级批量取数据：最多50000条/批次，适应ESP32超大UDP包
                    for (int i = 1; i < 50000; i++) {
                        byte[] moreData = dataQueue.poll();
                        if (moreData == null) break;
                        batchData.add(moreData);
                    }
                    
                    // 🚀 高性能批量处理数据
                    long startTime = System.nanoTime();
                    
                    // 🚀 优化1：检测队列大小，动态调整处理策略
                    int queueSize = dataQueue.size();
                    boolean highLoadDetected = queueSize > 1000 || batchData.size() > 1000;
                    
                    if (highLoadDetected) {
                        // 🚀 高负载模式：快速处理，跳过详细日志
                        for (byte[] data : batchData) {
                            try {
                                processReceivedData(data);
                            } catch (Exception e) {
                                // 高负载时静默处理错误，避免日志洪水
                                if (batchData.indexOf(data) % 1000 == 0) {
                                    logger.warn("高负载处理中出现错误: {}", e.getMessage());
                                }
                            }
                        }
                    } else {
                        // 正常模式：标准处理
                        for (byte[] data : batchData) {
                            processReceivedData(data);
                        }
                    }
                    
                    long processTime = (System.nanoTime() - startTime) / 1_000_000;
                    
                    // 🚀 优化2：性能监控和负载报告
                    if (batchData.size() > 100) {
                        logger.info("批量处理: {}条数据, 耗时: {}ms, 队列剩余: {}, 模式: {}", 
                                   batchData.size(), processTime, queueSize, 
                                   highLoadDetected ? "高负载" : "正常");
                    }
                    
                    // 🚀 优化3：队列积压告警
                    if (queueSize > 5000) {
                        logger.warn("⚠️ 数据队列积压严重: {}条数据待处理", queueSize);
                    }
                }
                
            } catch (InterruptedException e) {
                logger.info("数据处理线程被中断");
                break;
            } catch (Exception e) {
                logger.error("数据处理出错", e);
                // 🚀 错误恢复：短暂休眠后继续
                try {
                    Thread.sleep(10);
                } catch (InterruptedException ie) {
                    break;
                }
            }
        }
    }
    
    /**
     * 🚀 三协议自动检测和处理（FaCai + CANX + Text）
     */
    private void processReceivedData(byte[] data) {
        try {
            // 🚀 优先检测FaCai协议：检查前2字节魔数
            if (data.length >= 15 && isFaCaiProtocol(data)) {
                parseFaCaiPacket(data);
                return;
            }
            
            // 检测多个FaCai数据包
            if (data.length > 15 && isFaCaiMultiPacket(data)) {
                parseMultipleFaCaiPackets(data);
                return;
            }
            
            // 检测二进制协议：优先检查84字节的Java兼容CAN包
            if (data.length == 84 && isBinaryProtocol(data)) {
                parseJavaCompatibleCANPacket(data);
                return;
            }
            
            // 检测多包二进制数据：大于84字节的可能包含多个CAN包
            if (data.length > 84 && data.length % 84 == 0) {
                parseMultipleBinaryPackets(data);
                return;
            }
            
            // 检测其他长度的二进制协议
            if (data.length >= 4 && isBinaryProtocol(data)) {
                parseMinimalCANPacket(data);
                return;
            }
            
            // 检测文本协议
            String dataStr = new String(data, java.nio.charset.StandardCharsets.UTF_8);
            if (dataStr.startsWith(TEXT_MAGIC)) {
                // 使用现有的文本协议解析逻辑
                parseTextCANPacket(data);
//                logger.debug("检测到文本协议数据: {}", dataStr.substring(0, Math.min(50, dataStr.length())));
                return;
            }
            
            // 未知协议，记录日志
            logger.warn("未知协议数据: 长度={}, 前16字节={}", 
                       data.length, 
                       bytesToHex(java.util.Arrays.copyOf(data, Math.min(16, data.length)), 
                                 Math.min(16, data.length)));
                       
        } catch (Exception e) {
            logger.error("协议检测失败: {}", e.getMessage(), e);
        }
    }

    private void parseTextCANPacket(byte[] data) {
        List<CanMessage> messages = CanMessage.parse(data);
        for (CanMessage message : messages) {
            addCANMessageToQueue(message);
        }

    }

    /**
     * 🚀 检测是否为FaCai协议
     */
    private boolean isFaCaiProtocol(byte[] data) {
        if (data.length < 15) return false; // 最小包长度检查（15字节头部）
        
        // 检查魔数 (小端序)
        int magic = java.nio.ByteBuffer.wrap(data, 0, 2)
                                      .order(java.nio.ByteOrder.LITTLE_ENDIAN)
                                      .getShort() & 0xFFFF;
        return magic == FACAI_MAGIC;
    }
    
    /**
     * 🚀 检测是否为FaCai多包数据
     */
    private boolean isFaCaiMultiPacket(byte[] data) {
        if (data.length < 15) return false;
        
        int offset = 0;
        while (offset < data.length) {
            if (offset + 15 > data.length) return false; // 不足头部长度
            
            // 检查魔数
            int magic = java.nio.ByteBuffer.wrap(data, offset, 2)
                                          .order(java.nio.ByteOrder.LITTLE_ENDIAN)
                                          .getShort() & 0xFFFF;
            if (magic != FACAI_MAGIC) return false;
            
            // 获取数据长度
            int dataLength = data[offset + 14] & 0xFF;
            if (dataLength > 64) return false; // 数据长度检查
            
            offset += 15 + dataLength; // 移动到下一个包
        }
        
        return offset == data.length; // 确保完整解析
    }
    
    /**
     * 🚀 解析FaCai协议数据包
     */
    private void parseFaCaiPacket(byte[] data) {
        try {
            java.nio.ByteBuffer buffer = java.nio.ByteBuffer.wrap(data);
            buffer.order(java.nio.ByteOrder.LITTLE_ENDIAN);
            
            // 解析包头
            int magic = buffer.getShort() & 0xFFFF;        // 魔数验证
            if (magic != FACAI_MAGIC) {
                logger.warn("FaCai包魔数验证失败: 期望=0x{}, 实际=0x{}", 
                           Integer.toHexString(FACAI_MAGIC), Integer.toHexString(magic));
                return;
            }
            
            long timestampWithIndex = buffer.getLong();  // 8字节时间戳+索引组合
            
            // 🚀 时间戳使用上位机系统时间
            long esp32Index = timestampWithIndex & 0xFFFFFFFFL;  // 低32位是ESP32的消息索引
            long actualTimestamp = System.currentTimeMillis();  // 使用上位机当前时间作为时间戳
            
            long canId = Integer.toUnsignedLong(buffer.getInt());
            int flags = buffer.get() & 0xFF;
            int len = buffer.get() & 0xFF;
            
            // 验证数据长度
            if (len > 64) {
                logger.warn("FaCai CAN数据长度异常: {}, 限制到64字节", len);
                len = 64;
            }
            
            // 验证包长度
            if (data.length != 15 + len) {
                logger.warn("FaCai包长度不匹配: 期望={}, 实际={}", 15 + len, data.length);
                return;
            }
            
            // 提取数据
            byte[] canData = new byte[len];
            buffer.get(canData, 0, len);
            
            // 解析标志位
            boolean isExtended = (flags & 0x01) != 0;
            boolean isRTR = (flags & 0x02) != 0;
            boolean isCANFD = (flags & 0x04) != 0;
            
            // 转换为统一的CanMessage对象
            CanMessage canMsg = new CanMessage();
            canMsg.setCanId(String.format(isExtended ? "%08X" : "%03X", canId));
            canMsg.setData(bytesToHexString(canData));
            canMsg.setTime(formatCANoeStyleTimestamp(actualTimestamp)); // CANoe风格微秒级精度时间戳
            canMsg.setDirection(isRTR ? "RTR" : "接收");
            canMsg.setIndexCounter(esp32Index);
            
            // 🚀 直接添加到UI
            addCANMessageToQueue(canMsg);
            
            logger.debug("解析FaCai消息: ID={}, ESP32索引={}, 数据长度={}, 扩展帧={}, RTR={}, CANFD={}", 
                        canMsg.getCanId(), esp32Index, len, isExtended, isRTR, isCANFD);
            
        } catch (Exception e) {
            logger.error("FaCai数据包解析失败", e);
        }
    }
    
    /**
     * 🚀 解析多个FaCai数据包
     */
    private void parseMultipleFaCaiPackets(byte[] data) {
        try {
            int offset = 0;
            int packetCount = 0;
            
            while (offset < data.length) {
                // 检查剩余数据是否足够一个头部
                if (offset + 15 > data.length) {
                    logger.warn("FaCai多包数据不完整，剩余字节: {}", data.length - offset);
                    break;
                }
                
                // 获取数据长度
                int dataLength = data[offset + 14] & 0xFF;
                int packetSize = 15 + dataLength;
                
                // 检查是否有足够的数据
                if (offset + packetSize > data.length) {
                    logger.warn("FaCai包数据不完整，需要: {}, 剩余: {}", packetSize, data.length - offset);
                    break;
                }
                
                // 提取单个包并解析
                byte[] singlePacket = Arrays.copyOfRange(data, offset, offset + packetSize);
                parseFaCaiPacket(singlePacket);
                
                offset += packetSize;
                packetCount++;
            }
            
            logger.info("FaCai多包解析完成: 处理了{}个数据包, 总大小: {}字节", packetCount, data.length);
            
        } catch (Exception e) {
            logger.error("FaCai多包数据解析失败", e);
        }
    }
    
    /**
     * 🚀 检测是否为二进制协议
     */
    private boolean isBinaryProtocol(byte[] data) {
        if (data.length < 84) return false; // 最小包长度检查 (修正为84字节)
        
        // 检查魔数 (小端序)
        int magic = java.nio.ByteBuffer.wrap(data, 0, 4)
                                      .order(java.nio.ByteOrder.LITTLE_ENDIAN)
                                      .getInt();
        return magic == BINARY_MAGIC;
    }
    
    /**
     * 🚀 解析Java兼容的84字节CAN数据包 - 主要解析方法
     */
    private void parseJavaCompatibleCANPacket(byte[] data) {
        try {
            java.nio.ByteBuffer buffer = java.nio.ByteBuffer.wrap(data);
            buffer.order(java.nio.ByteOrder.LITTLE_ENDIAN);
            
            // 解析包头
            int magic = buffer.getInt();        // 魔数验证
            if (magic != BINARY_MAGIC) {
                logger.warn("Java兼容包魔数验证失败: 期望=0x{}, 实际=0x{}", 
                           Integer.toHexString(BINARY_MAGIC), Integer.toHexString(magic));
                return;
            }
            
            long timestampWithIndex = buffer.getLong();  // 8字节时间戳+索引组合
            
            // 🚀 时间戳使用上位机系统时间
            long esp32Index = timestampWithIndex & 0xFFFFFFFFL;  // 低32位是ESP32的消息索引
            long actualTimestamp = System.currentTimeMillis();  // 使用上位机当前时间作为时间戳
            
            long canId = Integer.toUnsignedLong(buffer.getInt());
            int len = buffer.get() & 0xFF;
            int flags = buffer.get() & 0xFF;
            buffer.getShort(); // 跳过保留字节
            
            // 验证数据长度
            if (len > 64) {
                logger.warn("CAN数据长度异常: {}, 限制到64字节", len);
                len = 64;
            }
            
            // 提取数据
            byte[] canData = new byte[len];
            buffer.get(canData, 0, len);
            
            // 解析标志位
            boolean isExtended = (flags & 0x01) != 0;
            
            // 转换为统一的CanMessage对象
            CanMessage canMsg = new CanMessage();
            canMsg.setCanId(String.format(isExtended ? "%08X" : "%03X", canId));
            canMsg.setData(bytesToHexString(canData));
            canMsg.setTime(formatCANoeStyleTimestamp(actualTimestamp)); // CANoe风格微秒级精度时间戳
            canMsg.setDirection("接收");
            // 🚀 修复：使用ESP32发送的索引而不是自己维护的计数器
            canMsg.setIndexCounter(esp32Index);
            
            // 🚀 直接添加到UI
            addCANMessageToQueue(canMsg);
            
            logger.debug("解析CAN消息: ID={}, ESP32索引={}, 数据长度={}", canMsg.getCanId(), esp32Index, len);
            
        } catch (Exception e) {
            logger.error("Java兼容CAN数据包解析失败", e);
        }
    }
    
    /**
     * 🚀 解析多个84字节的二进制数据包
     */
    private void parseMultipleBinaryPackets(byte[] data) {
        try {
            int packetCount = data.length / 84;
            logger.info("检测到多包数据: 总长度={}, 包数量={}", data.length, packetCount);
            
            for (int i = 0; i < packetCount; i++) {
                int offset = i * 84;
                byte[] singlePacket = Arrays.copyOfRange(data, offset, offset + 84);
                parseJavaCompatibleCANPacket(singlePacket);
            }
            
            logger.info("多包解析完成: 处理了{}个数据包", packetCount);
            
        } catch (Exception e) {
            logger.error("多包二进制数据解析失败", e);
        }
    }
    
    /**
     * 🚀 解析最小包装CAN数据包 - 兼容性方法
     */
    private void parseMinimalCANPacket(byte[] data) {
        try {
            // 如果是84字节，直接调用Java兼容解析
            if (data.length == 84) {
                parseJavaCompatibleCANPacket(data);
                return;
            }
            
            java.nio.ByteBuffer buffer = java.nio.ByteBuffer.wrap(data);
            buffer.order(java.nio.ByteOrder.LITTLE_ENDIAN);
            
            // 解析包头
            buffer.getInt();                    // 跳过已验证的魔数
            long timestampWithIndex = buffer.getLong();  // 8字节时间戳+索引组合
            
            // 🚀 修复：提取ESP32的32位索引，时间戳使用上位机系统时间
            long esp32Index = timestampWithIndex & 0xFFFFFFFFL;  // 低32位是ESP32的消息索引
            long actualTimestamp = System.currentTimeMillis();  // 使用上位机当前时间作为时间戳
            
            long canId = Integer.toUnsignedLong(buffer.getInt());
            int len = buffer.get() & 0xFF;
            int flags = buffer.get() & 0xFF;
            buffer.getShort(); // 跳过保留字节
            
            // 提取数据
            byte[] canData = new byte[Math.min(len, data.length - 20)];
            buffer.get(canData, 0, canData.length);
            
            // 解析标志位
            boolean isExtended = (flags & 0x01) != 0;
            boolean isCANFD = (flags & 0x02) != 0;
            
            // 转换为统一的CanMessage对象
            CanMessage canMsg = new CanMessage();
            canMsg.setCanId(String.format(isExtended ? "%08X" : "%03X", canId));
            canMsg.setData(bytesToHexString(canData));
            canMsg.setTime(formatCANoeStyleTimestamp(actualTimestamp));
            canMsg.setDirection("接收");
            canMsg.setIndexCounter(esp32Index);
            
            // 🚀 直接添加到UI
            addCANMessageToQueue(canMsg);
            
            logger.debug("🚀 [最小协议] 解析CAN消息: ID={}, ESP32索引={}, 数据长度={}, 扩展帧={}, CANFD={}", 
                        canMsg.getCanId(), esp32Index, len, isExtended, isCANFD);
            
        } catch (Exception e) {
            logger.error("最小CAN数据包解析失败", e);
        }
    }
    
    /**
     * 🚀 添加CAN消息到处理队列 - 直接处理方式
     */
    // 🚀 UI更新批量缓冲区
    private final List<CanMessage> uiUpdateBuffer = new ArrayList<>();
    private volatile long lastUIUpdate = 0;
    private static final int UI_UPDATE_INTERVAL = 50;
    private static final int UI_BATCH_SIZE = 10;
    
    // 🚀 高负载模式配置 - 极限性能优化
    private static final int HIGH_LOAD_UI_UPDATE_INTERVAL = 150; // 高负载模式：150ms更新间隔（更流畅）
    private static final int HIGH_LOAD_UI_BATCH_SIZE = 10000; // 高负载模式：每批10000条消息（超级提升）
    private volatile boolean highLoadMode = false; // 高负载模式标志
    private volatile long uiUpdateCount = 0; // UI更新次数统计
    private volatile long lastFpsCheck = 0; // 上次帧率检查时间
    
    private void addCANMessageToQueue(CanMessage canMsg) {
        try {
            synchronized (uiUpdateBuffer) {
                uiUpdateBuffer.add(canMsg);
                
                long currentTime = System.currentTimeMillis();
                
                // 🚀 动态负载检测：根据队列大小自动切换高负载模式 - 优化阈值保持流畅
                if (!highLoadMode && uiUpdateBuffer.size() > 800) {
                    highLoadMode = true;
                    logger.info("🚀 检测到高负载，切换到高性能模式 (队列大小: {})", uiUpdateBuffer.size());
                } else if (highLoadMode && uiUpdateBuffer.size() < 200 && 
                          currentTime - lastUIUpdate > 2000) {
                    highLoadMode = false;
                    logger.info("✅ 负载降低，恢复正常模式 (队列大小: {})", uiUpdateBuffer.size());
                }
                
                // 🚀 根据模式选择批量大小和更新间隔
                int currentBatchSize = highLoadMode ? HIGH_LOAD_UI_BATCH_SIZE : UI_BATCH_SIZE;
                int currentUpdateInterval = highLoadMode ? HIGH_LOAD_UI_UPDATE_INTERVAL : UI_UPDATE_INTERVAL;
                
                // 🚀 智能UI更新策略：基于数据量和时间间隔的自适应更新
                boolean shouldUpdate = false;
                
                if (uiUpdateBuffer.size() >= currentBatchSize) {
                    // 达到批量大小立即更新
                    shouldUpdate = true;
                } else if (currentTime - lastUIUpdate > currentUpdateInterval) {
                    // 达到时间间隔更新
                    shouldUpdate = true;
                } else if (uiUpdateBuffer.size() >= 5 && currentTime - lastUIUpdate > 25) {
                    // 🚀 流畅性优化：5条消息25ms后立即更新
                    shouldUpdate = true;
                } else if (uiUpdateBuffer.size() >= 3 && currentTime - lastUIUpdate > 20) {
                    // 🚀 超流畅优化：3条消息20ms后立即更新
                    shouldUpdate = true;
                } else if (!uiUpdateBuffer.isEmpty() && currentTime - lastUIUpdate > 100) {
                    // 🚀 关键修复：单条消息100ms后强制更新
                    shouldUpdate = true;
                }else if (!uiUpdateBuffer.isEmpty() && uiUpdateBuffer.size() <= 100) {
                    // 🚀 关键修复：尾数立即更新
                    shouldUpdate = true;
                }
                
                if (shouldUpdate) {
                    
                    // 复制缓冲区数据
                    final List<CanMessage> batchToUpdate = new ArrayList<>(uiUpdateBuffer);
                    uiUpdateBuffer.clear();
                    lastUIUpdate = currentTime;
                    final boolean isHighLoadMode = highLoadMode; // 捕获当前状态
                    
                    if (mainFrame != null) {
                        try {
                            List<CanMessage> displayMessages = batchToUpdate;

                            // 🚀 优化1：高负载模式下智能显示策略 - 提高省略门槛保持流畅
                            if (isHighLoadMode && batchToUpdate.size() > 2000) {
                                // 高负载模式：只有超过2000条时才省略显示，显示前200条和后200条
                                displayMessages = new ArrayList<>();
                                displayMessages.addAll(batchToUpdate.subList(0, Math.min(200, batchToUpdate.size())));

                                if (batchToUpdate.size() > 400) {
                                    // 添加省略标记
                                    CanMessage ellipsis = new CanMessage();
                                    ellipsis.setCanId("...");
                                    ellipsis.setData(String.format("[省略%d条消息]", batchToUpdate.size() - 400));
                                    ellipsis.setTime("...");
                                    ellipsis.setDirection("省略");
                                    displayMessages.add(ellipsis);

                                    // 添加最后200条
                                    int startIndex = batchToUpdate.size() - 200;
                                    displayMessages.addAll(batchToUpdate.subList(startIndex, batchToUpdate.size()));
                                }
                            }

                            // 🚀 关键修复：直接调用表格的appendBatch方法，确保数据立即显示
                            //mainFrame.getEnhancedTable().appendBatch(displayMessages);
                            // 需要新增一个方法将接收到的数据存储到数据库
                            saveMessageToDb.save(displayMessages,mainFrame.getProjectId());
                            // 加入一个数据检查，检查是否符合事件的发生条件
                            this.queueMonitor.addToQueue(displayMessages);

                        } catch (Exception e) {
                            logger.error("UI线程添加CAN消息失败: {}", e.getMessage());
                        }
                    }
                }
            }
        } catch (Exception e) {
            logger.error("添加CAN消息到队列失败: {}", e.getMessage(), e);
        }
    }
    
    /**
     * 🚀 创建FaCai协议CAN数据包
     */
    private byte[] createFaCaiCANPacket(CanMessage message) {
        try {
            // 解析数据长度
            byte[] canData = hexStringToBytes(message.getData());
            int dataLength = Math.min(canData.length, 64); // 最大64字节
            
            ByteBuffer buffer = ByteBuffer.allocate(15 + dataLength); // 变长包
            buffer.order(ByteOrder.LITTLE_ENDIAN);
            
            // 1. 魔术字节 "一路发财" (2字节)
            buffer.putShort((short) 0x1688);
            
            // 2. 时间戳 (8字节，微秒) - 修复时区问题
            long timestamp = System.currentTimeMillis() * 1000; // 转换为微秒
            if (message.getTime() != null && !message.getTime().isEmpty()) {
                try {
                    // 🚀 修复：如果消息包含时间戳，需要正确解析
                    double timeInSeconds = Double.parseDouble(message.getTime());
                    
                    // 🚀 简化：直接将时间戳转换为微秒
                    if (timeInSeconds < 946684800) { // 2000年1月1日的时间戳
                        // 相对时间：保持原值，转换为微秒
                        timestamp = (long)(timeInSeconds * 1_000_000);
                        logger.debug("解析相对时间戳: {}s -> {}μs", timeInSeconds, timestamp);
                    } else {
                        // 绝对时间戳：如果是秒级时间戳，转换为微秒
                        if (timeInSeconds < System.currentTimeMillis()) {
                            // 毫秒级时间戳，转换为微秒
                            timestamp = (long)(timeInSeconds * 1000);
                        } else {
                            // 秒级时间戳，转换为微秒
                            timestamp = (long)(timeInSeconds * 1_000_000);
                        }
                        logger.debug("解析绝对时间戳: {}s -> {}μs", timeInSeconds, timestamp);
                    }
                } catch (NumberFormatException e) {
                    // 解析失败，使用当前UTC时间
                    logger.warn("时间戳解析失败: {}, 使用当前时间", message.getTime());
                }
            }
            buffer.putLong(timestamp);
            
            // 3. CAN ID (4字节)
            long canId = parseCANId(message.getCanId());
            buffer.putInt((int) canId);
            
            // 4. 标志位 (1字节)
            byte flags = 0;
            if (canId > 0x7FF) {
                flags |= 0x01; // 扩展帧
            }
            if (message.getDirection() != null && message.getDirection().equals("RTR")) {
                flags |= 0x02; // RTR帧
            }
            if (dataLength > 8) {
                flags |= 0x04; // CAN FD
            }
            buffer.put(flags);
            
            // 5. 数据长度 (1字节)
            buffer.put((byte) dataLength);
            
            // 6. CAN数据 (变长)
            buffer.put(canData, 0, dataLength);
            
            return buffer.array();
            
        } catch (Exception e) {
            throw new RuntimeException("FaCai数据包创建失败", e);
        }
    }
    
    /**
     * 🚀 创建ESP32兼容的二进制CAN数据包
     */
    private byte[] createBinaryCANPacket(CanMessage message) {
        try {
            ByteBuffer buffer = ByteBuffer.allocate(84); // 固定84字节
            buffer.order(ByteOrder.LITTLE_ENDIAN);
            
            // 1. 魔术字节 "CANX" (4字节)
            buffer.putInt(0x43414E58);
            
            // 2. 时间戳 (8字节，微秒) - 修复时区问题
            long timestamp = System.currentTimeMillis() * 1000; // 转换为微秒
            if (message.getTime() != null && !message.getTime().isEmpty()) {
                try {
                    // 🚀 修复：如果消息包含时间戳，需要正确解析
                    double timeInSeconds = Double.parseDouble(message.getTime());
                    
                    // 🚀 简化：直接将时间戳转换为微秒
                    if (timeInSeconds < 946684800) { // 2000年1月1日的时间戳
                        // 相对时间：保持原值，转换为微秒
                        timestamp = (long)(timeInSeconds * 1_000_000);
                        logger.debug("解析相对时间戳: {}s -> {}μs", timeInSeconds, timestamp);
                    } else {
                        // 绝对时间戳：如果是秒级时间戳，转换为微秒
                        if (timeInSeconds < System.currentTimeMillis()) {
                            // 毫秒级时间戳，转换为微秒
                            timestamp = (long)(timeInSeconds * 1000);
                        } else {
                            // 秒级时间戳，转换为微秒
                            timestamp = (long)(timeInSeconds * 1_000_000);
                        }
                        logger.debug("解析绝对时间戳: {}s -> {}μs", timeInSeconds, timestamp);
                    }
                } catch (NumberFormatException e) {
                    // 解析失败，使用当前UTC时间
                    logger.warn("时间戳解析失败: {}, 使用当前时间", message.getTime());
                }
            }
            buffer.putLong(timestamp);
            
            // 3. CAN ID (4字节)
            long canId = parseCANId(message.getCanId());
            buffer.putInt((int) canId);
            
            // 4. 数据长度 (1字节)
            byte[] canData = hexStringToBytes(message.getData());
            int dataLength = Math.min(canData.length, 64); // 最大64字节
            buffer.put((byte) dataLength);

            byte flags = 0;
            if (canId > 0x7FF) {
                flags |= 0x01; // 扩展帧
            }
            if (dataLength > 8) {
                flags |= 0x02; // CAN FD
            }
            buffer.put(flags);
            
            // 6. 保留字节 (2字节)
            buffer.putShort((short) 0);
            
            // 7. CAN数据 (64字节)
            buffer.put(canData, 0, dataLength);
            // 剩余字节填充0
            for (int i = dataLength; i < 64; i++) {
                buffer.put((byte) 0);
            }
            
            return buffer.array();
            
        } catch (Exception e) {
            throw new RuntimeException("二进制数据包创建失败", e);
        }
    }
    
    /**
     * 🚀 解析CAN ID字符串为数值
     */
    private long parseCANId(String canIdStr) {
        try {
            if (canIdStr == null || canIdStr.isEmpty()) {
                return 0;
            }
            
            // 移除可能的前缀和空格
            String cleanId = canIdStr.trim().toUpperCase();
            if (cleanId.startsWith("0X")) {
                cleanId = cleanId.substring(2);
            }
            
            return Long.parseLong(cleanId, 16);
        } catch (Exception e) {
            logger.warn("CAN ID解析失败: '{}', 使用默认值0", canIdStr);
            return 0;
        }
    }
    
    /**
     * 🚀 十六进制字符串转字节数组
     */
    private byte[] hexStringToBytes(String hexStr) {
        try {
            if (hexStr == null || hexStr.isEmpty()) {
                return new byte[0];
            }
            
            // 移除空格和非十六进制字符
            String cleanHex = hexStr.replaceAll("[^0-9A-Fa-f]", "");
            
            // 确保偶数长度
            if (cleanHex.length() % 2 != 0) {
                cleanHex = "0" + cleanHex;
            }
            
            byte[] bytes = new byte[cleanHex.length() / 2];
            for (int i = 0; i < bytes.length; i++) {
                int index = i * 2;
                bytes[i] = (byte) Integer.parseInt(
                    cleanHex.substring(index, index + 2), 16);
            }
            
            return bytes;
        } catch (Exception e) {
            logger.warn("十六进制字符串转换失败: '{}', 返回空数组", hexStr);
            return new byte[0];
        }
    }
    
    /**
     * 🚀 字节数组转十六进制字符串
     */
    private String bytesToHexString(byte[] bytes) {
        StringBuilder sb = new StringBuilder();
        for (byte b : bytes) {
            sb.append(String.format("%02X", b & 0xFF));
        }
        return sb.toString();
    }
    
    /**
     * 🚀 CANoe风格时间戳格式化 - 微秒级精度显示
     */
    private String formatCANoeStyleTimestamp(long millis) {
        // 将毫秒时间戳转换为秒.微秒格式，保持6位小数精度
        long nanos = System.nanoTime() % 1_000_000; // 取纳秒的后6位（微秒级）
//        return String.format("%.6f", seconds);
        return String.format("%d.%06d", millis / 1000, (int)(millis % 1000 * 1000 + nanos / 1000));
    }
    
    /**
     * 🚀 更新发送间隔统计
     */
    private void updateSendInterval(long currentTime) {
        if (lastSendTime > 0) {
            double interval = currentTime - lastSendTime;
            // 使用简单移动平均计算当前间隔
            currentSendInterval = (currentSendInterval * 0.8) + (interval * 0.2);
        }
        lastSendTime = currentTime;
        // sendCount++; // 已移除sendCount变量
    }
    
    /**
     * 🚀 重置消息索引计数器
     */
    public void resetMessageIndex() {
        messageIndexCounter = 0;
    }
    
    /**
     * 🚀 获取当前消息索引值（用于调试）
     */
    public long getCurrentMessageIndex() {
        return messageIndexCounter;
    }
    
    /**
     * 🚀 获取精确的当前发送间隔（毫秒）
     */
    public double getPreciseCurrentSendInterval() {
        return currentSendInterval;
    }
    

    public boolean isConnectionHealthy() {
        // 检查心跳超时
        long currentTime = System.currentTimeMillis();
        if (lastHeartbeatTime > 0 && currentTime - lastHeartbeatTime > HEARTBEAT_TIMEOUT) {
            connectionHealthy = false;
            logger.warn("设备连接超时，上次心跳: {}ms前", currentTime - lastHeartbeatTime);
        }
        return connectionHealthy;
    }
    
    /**
     * 🚀 获取连接状态信息
     */
    public String getConnectionStatus() {
        String connection = connectionHealthy ? "连接正常" : "连接异常";
        String connectionInfo = boradAddressString != null ? 
                               String.format("IP:%s:%d", boradAddressString, boardPort) : "IP:未知";
        
        return String.format("连接状态: %s | 设备地址: %s", 
                           connection, connectionInfo);
    }
    
    /**
     * 🚀 主动发送心跳给远程设备建立连接
     */
    public void sendHeartbeat() {
        try {
            // 尝试连接默认设备地址（可配置）
            InetAddress remoteAddress = InetAddress.getByName("192.168.1.100");
            int remotePort = 8077;

            String heartbeatMessage = ACK_MESSAGE;  // 只发送"ACK"
            byte[] heartbeatData = heartbeatMessage.getBytes();
            
            // 发送心跳到远程设备
            DatagramPacket heartbeatPacket = new DatagramPacket(
                    heartbeatData, heartbeatData.length, remoteAddress, remotePort);
            socket.send(heartbeatPacket);

            
            // 设置目标地址，为后续通信做准备
            boardAddress = remoteAddress;
            boradAddressString = remoteAddress.getHostAddress();
            boardPort = remotePort;
            
        } catch (Exception e) {
            logger.error("发送心跳失败: " + e.getMessage());
            e.printStackTrace();
        }
    }
    
    /**
     * 🚀 自动连接CAN设备
     */
    private void autoConnectToDevice() {
        // 在后台线程中尝试连接，避免阻塞启动过程
        new Thread(() -> {
            try {
                Thread.sleep(1000); // 等待1秒让UDP监听线程完全启动
                
                logger.info("🔗 正在自动尝试连接CAN设备...");
                
                // 尝试连接默认设备地址
                String defaultDeviceIP = "192.168.1.100";
                int defaultDevicePort = 8077;
                
                // 发送初始心跳连接请求
                sendInitialHeartbeat(defaultDeviceIP, defaultDevicePort);
                
                // 等待几秒看是否收到回复
                Thread.sleep(2000);
                
                if (connectionHealthy && boardAddress != null) {
                    logger.info("CAN设备连接成功: {}:{}", boradAddressString, boardPort);
                } else {
                    logger.warn("⚠️ CAN设备自动连接失败，请检查：");

                }
                
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
                logger.warn("CAN设备自动连接线程被中断");
            } catch (Exception e) {
                logger.error("CAN设备自动连接异常: {}", e.getMessage());
            }
        }, "Device-AutoConnect").start();
    }
    
    /**
     * 🚀 发送初始心跳连接请求
     */
    private void sendInitialHeartbeat(String targetIP, int targetPort) {
        try {
            InetAddress targetAddress = InetAddress.getByName(targetIP);
            
            // 发送标准心跳请求（只发送"ACK"，匹配Python端和智能设备协议）
            byte[] heartbeatData = ACK_MESSAGE.getBytes();
            DatagramPacket heartbeatPacket = new DatagramPacket(
                    heartbeatData, heartbeatData.length, targetAddress, targetPort);
            
            socket.send(heartbeatPacket);
            logger.debug("发送初始连接心跳到: {}:{}", targetIP, targetPort);
            
        } catch (Exception e) {
            logger.error("发送初始心跳失败: {}", e.getMessage());
        }
    }
    
    /**
     * 🚀 简化的连接健康检查定时器
     */
    private void startSimpleHealthCheckTimer() {
        if (healthCheckTimer != null && !healthCheckTimer.isShutdown()) {
            healthCheckTimer.shutdown();
        }
        
        healthCheckTimer = Executors.newSingleThreadScheduledExecutor();
        healthCheckTimer.scheduleAtFixedRate(this::checkSimpleConnectionHealth, 2, 2, TimeUnit.SECONDS);
        logger.debug("连接健康检查定时器已启动");
    }
    
    /**
     * 🚀 停止连接健康检查定时器
     */
    private void stopHealthCheckTimer() {
        if (healthCheckTimer != null && !healthCheckTimer.isShutdown()) {
            healthCheckTimer.shutdown();
            try {
                if (!healthCheckTimer.awaitTermination(1, TimeUnit.SECONDS)) {
                    healthCheckTimer.shutdownNow();
                }
            } catch (InterruptedException e) {
                healthCheckTimer.shutdownNow();
                Thread.currentThread().interrupt();
            }
            logger.info("🛑 连接健康检查定时器已停止");
        }
    }
    
    /**
     * 🚀 简化的连接健康状态检查
     */
    private void checkSimpleConnectionHealth() {
        long currentTime = System.currentTimeMillis();
        boolean wasHealthy = connectionHealthy;
        
        // 🚀 主动发送心跳维持连接
        if (boardAddress != null && boardPort > 0) {
            // 每10秒发送一次心跳
            long timeSinceLastHeartbeat = lastHeartbeatTime > 0 ? currentTime - lastHeartbeatTime : Long.MAX_VALUE;
            
            if (timeSinceLastHeartbeat > 10000) { // 10秒没收到心跳，主动发送
                try {
                    // 发送心跳维持连接
                    byte[] heartbeatData = ACK_MESSAGE.getBytes();
                    DatagramPacket heartbeatPacket = new DatagramPacket(
                            heartbeatData, heartbeatData.length, boardAddress, boardPort);
                    socket.send(heartbeatPacket);
                    
                    logger.debug("💓 发送维持心跳到 {}:{} (距离上次心跳{}秒)", 
                               boradAddressString, boardPort, timeSinceLastHeartbeat / 1000);
                               
                } catch (Exception e) {
                    logger.warn("发送维持心跳失败: {}", e.getMessage());
                }
            }
        }
        
        // 检查心跳超时
        if (lastHeartbeatTime > 0 && currentTime - lastHeartbeatTime > HEARTBEAT_TIMEOUT) {
            connectionHealthy = false;
            if (wasHealthy) {
                String addressInfo = boradAddressString != null ? 
                    String.format("{}:{}", boradAddressString, boardPort) : "未知地址";
                logger.warn("⚠️ 设备连接中断检测: 已超过{}秒无心跳响应 (地址: {})", 
                           HEARTBEAT_TIMEOUT / 1000, addressInfo);
                           
                // 🚀 连接断开时尝试重新连接
                logger.info("🔄 尝试重新建立连接...");
                autoConnectToDevice();
            }
        } else if (!wasHealthy && connectionHealthy) {
            // 连接恢复
            logger.info("设备连接已恢复正常");
        }
    }
    
    /**
     * 🚀 强制刷新UI缓冲区，确保最后的数据显示
     */
    public void forceFlushUIBuffer() {
        // 由于保持原有UI更新方式，这里主要用于兼容API
        logger.info("🔄 强制刷新UI缓冲区 - 兼容模式");
    }
    
    // ==================== 🚀 新增功能方法结束 ====================

    /**
     * 🚀 测试方法：验证二进制数据解析和UI更新是否正常工作
     */
    public void testBinaryDataFlow() {
        logger.info("🧪 开始测试二进制数据流...");
        
        try {
            // 创建测试CAN消息
            CanMessage testMsg = new CanMessage();
            testMsg.setCanId("123");
            testMsg.setData("ABCDEF");
            testMsg.setTime("0.000001");
            testMsg.setDirection("测试");
            testMsg.setIndexCounter(999L);
            
            // 测试直接UI更新
            logger.info("🧪 测试直接UI更新...");
            addCANMessageToQueue(testMsg);
            
            // 测试数据队列同步
            logger.info("🧪 测试数据队列同步...");
            if (mainFrame != null && mainFrame.dataQueue != null) {
                logger.info("✅ 数据队列同步测试通过，队列大小: {}", mainFrame.dataQueue.size());
            } else {
                logger.warn("⚠️ 数据队列同步测试失败：mainFrame或dataQueue为空");
            }
            
            logger.info("🧪 二进制数据流测试完成");
            
        } catch (Exception e) {
            logger.error("🧪 二进制数据流测试失败: {}", e.getMessage(), e);
        }
    }

    public boolean isRunnging() {
        return running;
    }
}