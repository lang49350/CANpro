package org.example.utils.web;

import com.baomidou.mybatisplus.core.conditions.query.QueryWrapper;
import com.baomidou.mybatisplus.core.conditions.update.UpdateWrapper;
import jdk.nashorn.internal.runtime.logging.Logger;
import lombok.extern.slf4j.Slf4j;
import org.apache.commons.collections4.ListUtils;
import org.example.mapper.EventRecordMapper;
import org.example.mapper.ValueChangConfigMapper;
import org.example.pojo.data.CanMessage;
import org.example.pojo.data.EventRecord;
import org.example.pojo.data.ValueChangConfig;
import org.example.utils.db.DatabaseManager;
import org.example.utils.event.EventManager;
import org.example.utils.event.EventString;

import java.util.*;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;

/**
 * 数据队列监控器，用于检测队列长时间无数据的情况
 */
@Slf4j
public class DataQueueMonitor {
    private final Queue<byte[]> dataQueue;

    private Map<String,Queue<CanMessage>> canMessageQueue = new HashMap<>();
    private Thread monitorThread;
    private boolean isRunning;
    private long lastDataTime; // 最后一次有数据的时间戳
    private boolean eventTriggered; // 事件是否已触发的标志

    private int triggerCount;
    private Boolean queueActived;

    private UdpWifiDataReceiver uppWifiDataReceiver;
    private long projectId = 0; // 用于USB模式

    // 定义监控间隔和超时时间(10秒)
    private static final long CHECK_INTERVAL = 100; // 1秒检查一次
    private static final long TIMEOUT = 5000; // 10秒超时

    private List<ValueChangConfig> eventConfigs;

    private Map<String,String> msgValueMap = new HashMap<>();

    public DataQueueMonitor(Queue<byte[]> dataQueue) {
        this.dataQueue = dataQueue;
        this.lastDataTime = System.currentTimeMillis();
        this.eventTriggered = false;
        this.triggerCount = 0;
        queueActived = false;
        loadEventConfig();
    }
    
    /**
     * 设置项目ID（用于USB模式）
     */
    public void setProjectId(long projectId) {
        this.projectId = projectId;
    }
    
    /**
     * 获取项目ID
     */
    private long getProjectId() {
        if (uppWifiDataReceiver != null) {
            return uppWifiDataReceiver.getProjectId();
        }
        return projectId;
    }

    private void loadEventConfig() {
        DatabaseManager databaseManager = new DatabaseManager();
        List<ValueChangConfig> configs = databaseManager.execute(ValueChangConfig.class, (session) -> {
            ValueChangConfigMapper mapper = session.getMapper(ValueChangConfigMapper.class);
            QueryWrapper<ValueChangConfig> wrapper = new QueryWrapper<>();
            wrapper.lambda().eq(ValueChangConfig::getIsEnable, 1);
            return mapper.selectList(wrapper);
        });
        this.eventConfigs = configs;
    }

    public void setLastDataTime(long lastDataTime){
        this.lastDataTime = lastDataTime;
    }

    public void setQueueActived(Boolean queueActived){
        this.queueActived = queueActived;
    }


    public void resetTriggered(){
        this.eventTriggered = false;
    }
    public void setUppWifiDataReceiver(UdpWifiDataReceiver uppWifiDataReceiver){
        this.uppWifiDataReceiver = uppWifiDataReceiver;
    }

    /**
     * 开始监控队列
     */
    public void startMonitoring() {
        if (isRunning) {
            return;
        }

        isRunning = true;
        monitorThread = new Thread(this::monitorQueue);
        monitorThread.setDaemon(true); // 守护线程，随主线程退出
        monitorThread.start();
        //先清空接收数据量
        ReceivedDataReCorder.getInstance().clearProjectReceivedDataCount(getProjectId());
    }

    /**
     * 停止监控队列
     */
    public void stopMonitoring() {
        isRunning = false;
        if (monitorThread != null) {
            monitorThread.interrupt();
            monitorThread = null;
        }
        // 数据库中执行记录设置为删除
        DatabaseManager databaseManager = new DatabaseManager();
        databaseManager.execute(EventRecord.class,session -> {
            EventRecordMapper mapper = session.getMapper(EventRecordMapper.class);
            // 设置全部时间为删除
            mapper.update(new EventRecord(),new UpdateWrapper<EventRecord>()
                    .lambda().set(EventRecord::getIsDel,1));
            return null;
        });
    }

    /**
     * 监控队列的核心方法
     */
    private void monitorQueue() {

        while (isRunning) {
            try {
                // 计算速率
                calcSpeed();
                // 检查队列状态
                synchronized (dataQueue) { // 同步访问队列，保证线程安全
                        // 队列空，检查是否超时
                        long currentTime = System.currentTimeMillis();
                        long idleTime = currentTime - lastDataTime;
                        if (idleTime >= TIMEOUT && !eventTriggered && queueActived) {
                            // 满足条件，触发事件A
                            triggerEventA();
                            eventTriggered = true; // 防止重复触发
                            triggerCount++;
                        }
                }

                // 间隔一段时间再检查
                TimeUnit.MILLISECONDS.sleep(CHECK_INTERVAL);
            } catch (InterruptedException e) {
                // 线程被中断，退出循环
                Thread.currentThread().interrupt();
                break;
            } catch (Exception e) {
                e.printStackTrace();
            }

            // 检查队列看看是否有值变化事件发生
            if(!ListUtils.emptyIfNull(eventConfigs).isEmpty()){
                checkDataQueue();
            }
        }
    }

    private void calcSpeed() {
        long count = ReceivedDataReCorder.getInstance()
                .getProjectReceivedDataCount(getProjectId());
        //count 是字节数
        long bits = count * 8;
        // 计算速率，单位：bit/s M bit / s
        double speed = (double) (bits * (1000 / CHECK_INTERVAL)) /(1024*1024);
        // 记录速率
        ReceivedDataReCorder.getInstance().setProjectReceivedSpeedMap(getProjectId(), speed);
        // 清空数据接收大小记录
        ReceivedDataReCorder.getInstance().clearProjectReceivedDataCount(getProjectId());
//        log.info("***********************************接收数据大小****************************************" + count);
//        log.info("***********************************实时接收数据速率****************************************" +
//                ReceivedDataReCorder.getInstance().getEveSpeed(this.uppWifiDataReceiver.getProjectId()) + " M bit/s");
//        log.info("***********************************平均接收数据数量****************************************" +
//                ReceivedDataReCorder.getInstance().getMaxSpeed(this.uppWifiDataReceiver.getProjectId()));

    }

    /**
     * 触发事件A
     */
    private void triggerEventA() {
        // 使用SwingUtilities确保事件在EDT线程中执行
        javax.swing.SwingUtilities.invokeLater(() -> {
            System.out.println("***********************************事件发生****************************************");
            EventManager.getInstance().notifyListeners(
                    EventString.DATA_QUEUE_IDLE.name() // 假设定义了DATA_QUEUE_IDLE事件
            );
        });
    }

    // 检查数据是否符合事件触发的条件
    public void checkDataQueue() {
        for (ValueChangConfig eventConfig : eventConfigs) {
            Queue<CanMessage> canMessages = canMessageQueue.get(eventConfig.getTriggerMsgId());
            if (canMessages == null || canMessages.isEmpty()) {
                log.info(eventConfig.getTriggerMsgId() + "***********************************数据队列为空****************************************");
                continue;
            }
            Optional<CanMessage> fromMsg = canMessages.stream()
                    .filter(o -> o.getData().equals(eventConfig.getValueFrom())).findFirst();
            // 不含触发消息，直接跳过
            if(!fromMsg.isPresent()){
                // 清理队列
                canMessages.clear();
                log.info(eventConfig.getTriggerMsgId() + "***********************************数据队列已清空****************************************");
                continue;
            }
            // 包含触发消息，跳过
            Optional<CanMessage> toMsg = canMessages.stream()
                    .filter(o -> o.getData().equals(eventConfig.getValueTo())).findFirst();
            if(!toMsg.isPresent()){
                log.info(eventConfig.getTriggerMsgId() + "***********************************数据队列中不含To目标值****************************************");
                continue;
            }
            // 包含触发消息,比较是否先有from 后有 to
            String timeFrom = fromMsg.get().getTime();
            String timeTo = toMsg.get().getTime();
            //时间戳字符串转换为时间戳
            double timeFromLong = Double.parseDouble(timeFrom);
            double timeToLong = Double.parseDouble(timeTo);
            if(timeFromLong < timeToLong){
                // 满足条件，触发事件B
                System.out.println("***********************************值变更事件发生****************************************");
                triggerEventValueChanged(eventConfig);
                canMessages.clear();
            }
        }
    }

    private void triggerEventValueChanged(ValueChangConfig eventConfig) {
        javax.swing.SwingUtilities.invokeLater(() -> {
            System.out.println("***********************************值变更事件发生****************************************");
            EventManager.getInstance().notifyListeners(
                    EventString.VALUE_CHANGED.name() + "#" + eventConfig.getId()
            );
        });
    }

    public void addToQueue(List<CanMessage> displayMessages) {
        if(ListUtils.emptyIfNull(eventConfigs).isEmpty()){
            return;
        }
        // 检查当前待添加的数据中是否包含目标数据
        List<String> canIds = ListUtils.emptyIfNull(displayMessages).stream().map(o -> o.getCanId())
                .distinct().collect(Collectors.toList());
        List<String> trggerIds = eventConfigs.stream().map(o -> o.getTriggerMsgId()).collect(Collectors.toList());
        if(hasOverlap(canIds,trggerIds)){
            // 将符合ID的数据加入队列
            for (ValueChangConfig eventConfig : eventConfigs) {
                List<CanMessage> collect = displayMessages.stream().filter(o -> o.getCanId().equals(eventConfig.getTriggerMsgId()))
                        .collect(Collectors.toList());
                if(collect.isEmpty()){
                    continue;
                }
                Queue<CanMessage> messageQueue = canMessageQueue.get(eventConfig.getTriggerMsgId());
                if(messageQueue == null) {
                    messageQueue = new ConcurrentLinkedQueue<>(collect);
                    canMessageQueue.put(eventConfig.getTriggerMsgId(),messageQueue);
                }else{
                    messageQueue.addAll(collect);
                }
            }
        }

    }

    /**
     * 检查两个List<String>是否存在重叠元素（最优实现）
     * @param list1 第一个列表
     * @param list2 第二个列表
     * @return 存在重叠返回true，否则返回false
     */
    public static boolean hasOverlap(List<String> list1, List<String> list2) {
        // 避免空指针异常
        if (list1 == null || list2 == null || list1.isEmpty() || list2.isEmpty()) {
            return false;
        }

        // 选择较小的列表转换为HashSet，减少内存占用和转换时间
        List<String> smallerList = list1.size() <= list2.size() ? list1 : list2;
        List<String> largerList = list1.size() > list2.size() ? list1 : list2;

        // 转换为HashSet，实现O(1)查询
        Set<String> smallerSet = new HashSet<>(smallerList);

        // 遍历较大的列表，检查是否有元素存在于Set中
        for (String element : largerList) {
            if (smallerSet.contains(element)) {
                return true; // 找到重叠元素，立即返回
            }
        }

        // 无重叠元素
        return false;
    }

    // 可以定义一个事件监听器接口供外部实现
    public interface EventAListener {
        void onEventA();
    }
}