package org.example.utils.web;


import lombok.Data;
import lombok.extern.slf4j.Slf4j;
import org.apache.poi.ss.formula.functions.T;
import org.example.mapper.CanMessageMapper;
import org.example.pojo.data.CanMessage;
import org.example.service.CanMessageService;
import org.example.utils.db.DatabaseManager;
import org.example.utils.db.EntityTableGenerator;
import org.example.utils.db.InitAllTable;
import org.example.utils.db.TableNameContextHolder;
import org.example.utils.event.EventManager;
import org.example.utils.event.EventString;
import org.w3c.dom.events.EventException;

import java.util.ArrayList;
import java.util.List;
import java.util.Queue;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicLong;

@Data
@Slf4j
public class SaveMessageToDb {

    private  Boolean checked = false;
    private  DatabaseManager databaseManager = new DatabaseManager();
    private  String suffix_prefix = "_p_";
    // 缓存队列
    private  Queue<CanMessage> messagesQueue = new ConcurrentLinkedQueue<>();
    private  Integer batchSize = 1000;
    private AtomicLong recordCount = new AtomicLong(0);

    private Long projectId;
    private ScheduledExecutorService scheduler;

    public SaveMessageToDb() {
        // 初始化调度器（可根据需求调整线程池大小）
        scheduler = Executors.newSingleThreadScheduledExecutor();
        // 启动定时任务
        startScheduledTask();
    }

    private void startScheduledTask() {
        // 定义任务逻辑
        Runnable task = this::executeTask;

        // 安排任务：延迟1秒开始，每10秒执行一次
        scheduler.scheduleAtFixedRate(
                task,
                500,  // 初始延迟
                500, // 执行间隔
                TimeUnit.MILLISECONDS
        );
    }

    private void executeTask() {
        if(projectId == null){
            return;
        }
        long currentCount = recordCount.get(); // 获取当前值（线程安全）
        // 过滤无效值（根据业务调整，比如排除0）
        if (currentCount < 1) {
            return;
        }
        EventManager.getInstance().notifyListeners(
                EventString.REFRESS_RECEIVE_DATA_COUNTER.name() + "#" + projectId + "#" + recordCount.get()
        );
    }


    public  void save(List<CanMessage> messages, long projectId) {
        if(!checked){
            checkAndCreateTable(projectId);
        }
        messages.forEach(message -> {
            message.setProjectId(projectId);
        });
        if(messagesQueue.size() <= batchSize){
            messagesQueue.addAll(messages);
            // check if contains message for current drawing chart .if true, add to chartCacher
            checkAndCatchChartData(messages);
            recordCount.addAndGet(messages.size());
        }else{
            messagesQueue.addAll(messages);
            recordCount.addAndGet(messages.size());
            saveFromQueue(projectId);
        }

    }

    private void checkAndCatchChartData(List<CanMessage> messages) {
        ChartDataCacher instance = ChartDataCacher.getInstance();
        for (CanMessage message : messages) {
            boolean needed = instance.isMessageNeeded(projectId, message.getCanId());
            if( needed ){
                instance.addData(projectId, message.getCanId(), message);
            }
        }

    }

    private void saveFromQueue(long projectId) {
        Integer result = databaseManager.execute(CanMessage.class, (session) -> {
            TableNameContextHolder.setSuffix("can_message", suffix_prefix + projectId);
            try{
                // 从队列中取出1000条数据存储
                long start = System.nanoTime();
                List<CanMessage> canMessages = fetchMessages();
                int i = session.getMapper(CanMessageMapper.class).insertBatch(canMessages);
                long end = System.nanoTime();
                // 毫秒
                log.info("插入 {} 条数据耗时 {} 毫秒", canMessages.size(), (end - start) / 1_000_000);
                log.info("dataQueue size: {}", messagesQueue.size());
                return i;
            }finally {
                TableNameContextHolder.clearSuffix("can_message");
            }
        });
    }

    public  void checkAndCreateTable(long projectId) {
        if(checked){
            return;
        }
        this.projectId = projectId;
        InitAllTable.init();
        String suffix = suffix_prefix + projectId;
        boolean tableExists = databaseManager.tableExists("can_message" + suffix, CanMessage.class);
        System.out.println("can_message" + suffix + " 存在吗 :" +tableExists);
        if(!tableExists){
            String sql = EntityTableGenerator.generateCreateTableSql(CanMessage.class, suffix);
            databaseManager.createTable(EntityTableGenerator.getDatabaseName(CanMessage.class),sql);
        }
        checked = true;
    }

    private  List<CanMessage> fetchMessages(){
        List<CanMessage> result = new ArrayList<>(batchSize); // 预指定容量
        int count = 0;
        CanMessage msg;
        // 循环条件尽可能简单
        while (count < batchSize && (msg = messagesQueue.poll()) != null) {
            result.add(msg);
            count++;
        }
        return result;
    }

    public void stopTask() {
        if (scheduler != null && !scheduler.isShutdown()) {
            scheduler.shutdown(); // 优雅关闭
            try {
                // 等待任务结束，超时则强制关闭
                if (!scheduler.awaitTermination(1, TimeUnit.SECONDS)) {
                    scheduler.shutdownNow();
                }
            } catch (InterruptedException e) {
                scheduler.shutdownNow();
            }
        }
    }

    @Override
    protected void finalize() throws Throwable {
        super.finalize();
        stopTask(); // 垃圾回收前尝试关闭任务
    }
    
    // 将队列中的数据中的剩余数据全量的存储到数据库
    public void saveAll() {
        while (messagesQueue.size()>batchSize){
            saveFromQueue(projectId);
        }
        saveFromQueue(projectId);

    }

    public void resetCounter(){
        this.recordCount.set(0L);
    }
}
