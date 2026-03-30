package org.example.ui.project.send;

import com.baomidou.mybatisplus.core.conditions.query.QueryWrapper;
import lombok.Data;
import lombok.extern.slf4j.Slf4j;
import org.apache.commons.collections4.ListUtils;
import org.apache.commons.lang3.StringUtils;
import org.apache.poi.ss.formula.functions.T;
import org.example.mapper.EventConfigMapper;
import org.example.mapper.EventRecordMapper;
import org.example.mapper.ValueChangConfigMapper;
import org.example.pojo.data.CanMessage;
import org.example.pojo.data.EventConfig;
import org.example.pojo.data.EventRecord;
import org.example.pojo.data.ValueChangConfig;
import org.example.utils.db.DatabaseManager;
import org.example.utils.event.EventListener;
import org.example.utils.event.EventManager;
import org.example.utils.event.EventString;
import org.example.utils.web.UdpWifiDataReceiver;
import org.example.utils.web.UdpWifiReceiverManager;
import org.springframework.ui.context.Theme;

import javax.swing.*;
import java.text.SimpleDateFormat;
import java.util.*;


@Slf4j
@Data
public class DoEventAction implements EventListener {

    private UdpWifiDataReceiver dataReceiver;

    private DatabaseManager databaseManager;

    private static Map<Long,Long> records = new HashMap<>();

    private List<ValueChangConfig> configs = new ArrayList<>() ;
    public DoEventAction(UdpWifiDataReceiver receiver) {
        EventManager.getInstance().addListener(this);
        databaseManager = new DatabaseManager();
        dataReceiver = receiver;
        DatabaseManager databaseManager = new DatabaseManager();
        reloadValueChangeConfig();

    }
    @Override
    public void onEvent(String event) {
        System.out.println("DoEventAction: " + event);
        if(event.startsWith(EventString.DATA_QUEUE_IDLE.name())) {
            // 启动一个新的线程去发送数据，发送后销毁线程
            new Thread(this::doEvent).start();

        }
        if(event.startsWith(EventString.VALUE_CHANGED.name())) {
            // 启动一个新的线程去发送数据，发送后销毁线程
            new Thread(()->{
                doEventValueChanged(event.split("#")[1]);
            }).start();
        }
        if(event.startsWith(EventString.UPDATE_VALUE_CHANGE_CONFIG.name())) {
            reloadValueChangeConfig();
        }
    }

    private void reloadValueChangeConfig() {
        List<ValueChangConfig> configs1 = databaseManager.execute(ValueChangConfig.class, (session) -> {
            ValueChangConfigMapper mapper = session.getMapper(ValueChangConfigMapper.class);
            QueryWrapper<ValueChangConfig> objectQueryWrapper = new QueryWrapper<>();
            objectQueryWrapper.lambda().eq(ValueChangConfig::getIsEnable, 1);
            return mapper.selectList(objectQueryWrapper);
        });
        configs.clear();
        configs.addAll(configs1);
    }

    private void doEventValueChanged(String valueChangedConfigId) {
        Optional<ValueChangConfig> first = ListUtils.emptyIfNull(configs).stream().filter(o -> o.getId().equals(Long.parseLong(valueChangedConfigId)))
                .findFirst();
        if(!first.isPresent()){
            log.info("value change event not found");
            return;
        }
        ValueChangConfig config = first.get();
        long maxCount = Long.parseLong(StringUtils.defaultIfBlank(config.getMaxTriggerCount(),"1"));
        Long record = records.get(config.getId()) == null?0L:records.get(config.getId());
        if(record>=maxCount){
            log.info("max value change event reached");
            return;
        }
        // 检查config
        String isCircleSend1 = config.getIsCircleSend1();
        //检查是否循环发
        if(isCircleSend1.equals("1")){
            circleSend(config);
        }else{
            // 发送一次
            sendOnce(config);
        }
        if(record == 0L){
            records.put(config.getId(),1L);
        }else{
            Long l = records.get(config.getId());
            records.put(config.getId(),l+1);
        }
    }

    private void sendOnce(ValueChangConfig config) {
        log.info("************************************ 单次发送" + config.getMessageSendOnWait1());
        String waitMs1 = StringUtils.defaultIfBlank(config.getEventWaitMs1(), "0");
        waitMs(waitMs1);
        CanMessage canMessage = new CanMessage();
        String[] split = config.getMessageSendOnWait1().split("_");
        if(split.length<2){
            JOptionPane.showMessageDialog(null, "正在执行值变化后触发的事件:[" +
                    config.getName()
                    +"],请检查您配置的发送数据格式是否正确");
            return;
        }
        canMessage.setCanId(split[0]);
        canMessage.setData(split[1]);
        if(StringUtils.isBlank(config.getPort())){
            dataReceiver.sendData(canMessage);
        }else{
            sendByDefinedPort(canMessage,config.getPort());
        }

        log.info("************************************ 单次发送" + config.getMessageSendOnWait1() + " 完成");
        if(config.getMessageSendOnWait2() != null) {
            String waitMs2 = StringUtils.defaultIfBlank(config.getEventWaitMs2(), "0");
            waitMs(waitMs2);
        }
        if(StringUtils.isBlank(config.getMessageSendOnWait2())){
            return;
        }
        CanMessage canMessage2 = new CanMessage();
        String[] split2 = config.getMessageSendOnWait2().split("_");
        canMessage2.setCanId(split2[0]);
        canMessage2.setData(split2[1]);
        if(StringUtils.isBlank(config.getPort())){
            dataReceiver.sendData(canMessage2);
        }else{
            sendByDefinedPort(canMessage2,config.getPort());
        }

        log.info("************************************ 单次发送" + config.getMessageSendOnWait2() + " 完成");

    }

    private void sendByDefinedPort(CanMessage canMessage, String port) {
        if(!UdpWifiReceiverManager.getInstance().hasReceiver(Integer.parseInt(port))){
            JOptionPane.showMessageDialog(null, "正在执行值变化后触发的事件:[" +
                    "数据将要从" + port + "发送"
                    +"],但是当前端口未启动，请先启动端口");
            return;
        }
        UdpWifiDataReceiver receiver = UdpWifiReceiverManager.getInstance().getReceiver(Integer.parseInt(port));
        boolean isRunning = receiver.isRunnging();
        if(!isRunning){
            JOptionPane.showMessageDialog(null, "正在执行值变化后触发的事件:[" +
                    "数据将要从" + port + "发送"
                    +"],但是当前端口未启动，请先启动端口");
            return;
        }
        receiver.sendData(canMessage);


    }

    private void circleSend(ValueChangConfig config) {
        log.info("************************************ 循环发送" + config.getMessageSendOnWait1());
        String maxCount = StringUtils.defaultIfBlank(config.getSendMaxCount1(), "0");
        int max = Integer.parseInt(maxCount);
        int count = 0;
        String waitMs1 = StringUtils.defaultIfBlank(config.getEventWaitMs1(), "0");
        waitMs(waitMs1);
        while (count<max) {
            sendOnce(config);
            count++;
            waitMs(config.getCircleTimeMs1());
        }
    }

    private void waitMs(String ms) {
        try {
            Thread.sleep(Long.parseLong(ms));
        } catch (InterruptedException e) {
            log.error("Error while waiting", e);
        }
    }

    public void doEvent() {
        List<EventConfig> eventConfigs = databaseManager.execute(EventConfig.class, session -> {
            EventConfigMapper mapper = session.getMapper(EventConfigMapper.class);
            return mapper.selectList(new QueryWrapper<>());
        });
        System.out.println("************************ eventConfigs: " + eventConfigs.size());
        for (EventConfig eventConfig : eventConfigs) {
            System.out.println("************************ eventConfig: " + eventConfig);
            if(eventConfig.getIsEnable().equals(1)) {
                // 等待1
                String waitTime = eventConfig.getWaitTime();
                if(StringUtils.isNotBlank(waitTime)) {
                    try {
                        Thread.sleep(Long.parseLong(waitTime));
                        System.out.println("等待时间1，" + waitTime);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }
                // 等待2
                String waitTime2 = eventConfig.getWaitTime2();
                if(StringUtils.isNotBlank(waitTime2)) {
                    try {
                        Thread.sleep(Long.parseLong(waitTime2));
                        System.out.println("等待时间2，" + waitTime2);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }
                // 计算Delta
                String deltaTime = eventConfig.getDeltaTime();
                Long realDeltaTime = 0L;
                EventRecord record = databaseManager.execute(EventRecord.class, session -> {
                    EventRecordMapper mapper = session.getMapper(EventRecordMapper.class);
                    QueryWrapper<EventRecord> wrapper = new QueryWrapper<>();
                    wrapper.lambda().eq(EventRecord::getName, eventConfig.getName())
                            .eq(EventRecord::getIsDel, 0);
                    List<EventRecord> eventRecords = mapper.selectList(wrapper);
                    // 查找running_times 最大的那条数据
                    Optional<EventRecord> max = ListUtils.emptyIfNull(eventRecords).stream()
                            .max(Comparator.comparingInt(EventRecord::getRunningTimes));
                    return max.orElse(null);
                });
                // 如果record 是不空的
                if(record != null) {
                    long l = Long.parseLong(deltaTime);
                    Integer runningTimes = record.getRunningTimes();
                    realDeltaTime = l * runningTimes;
                }
                try {
                    Thread.sleep(realDeltaTime);
                    System.out.println("等待时间 Delta，" + realDeltaTime);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
                // 发送消息
                String messageSendOnWait1 = eventConfig.getMessageSendOnWait1();
                if(StringUtils.isNotBlank(messageSendOnWait1)) {
                    String[] strings = messageSendOnWait1.split(",");
                    for (String s : strings) {
                        String[] split = s.split("_");
                        CanMessage canMessage = new CanMessage();
                        canMessage.setCanId(split[0]);
                        canMessage.setData(split[1]);
                        dataReceiver.sendData(canMessage);
                    }
                }
                //等待 3
                String waitTime3 = eventConfig.getWaitTime3();
                if(StringUtils.isNotBlank(waitTime3)) {
                    try{
                        Thread.sleep(Long.parseLong(waitTime3));
                        System.out.println("等待时间 3，" + waitTime3);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }
                // 发送消息2
                String messageSendOnWait2 = eventConfig.getMessageSendOnWait2();
                if(StringUtils.isNotBlank(messageSendOnWait2)) {
                    String[] strings = messageSendOnWait2.split(",");
                    for (String s : strings) {
                        String[] split = s.split("_");
                        CanMessage canMessage = new CanMessage();
                        canMessage.setCanId(split[0]);
                        canMessage.setData(split[1]);
                        dataReceiver.sendData(canMessage);
                    }
                }
                // 存库
                saveRecor (eventConfig);
            }

        }
    }

    private void saveRecor(EventConfig eventConfig) {
        int maxCounter = getMaxRunningCounter(eventConfig);
        EventRecord eventRecord = new EventRecord();
        eventRecord.setName(eventConfig.getName());
        eventRecord.setRunningTimes(maxCounter + 1);
        SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
        eventRecord.setRunningTime(sdf.format(new Date()));
        eventRecord.setIsDel(0);
        databaseManager.execute(EventRecord.class, session -> {
            EventRecordMapper mapper = session.getMapper(EventRecordMapper.class);
            mapper.insert(eventRecord);
            return null;
        });
    }

    private int getMaxRunningCounter(EventConfig eventConfig) {
        return databaseManager.execute(EventRecord.class, session -> {
            EventRecordMapper mapper = session.getMapper(EventRecordMapper.class);
            QueryWrapper<EventRecord> wrapper = new QueryWrapper<>();
            wrapper.lambda().eq(EventRecord::getName, eventConfig.getName())
                    .eq(EventRecord::getIsDel, 0)
                    .orderByDesc(EventRecord::getRunningTime);
            List<EventRecord> eventRecords = mapper.selectList(wrapper);
            if (!ListUtils.emptyIfNull(eventRecords).isEmpty()) {
                return eventRecords.get(0).getRunningTimes();
            }
            return 0;
        });
    }
}
