package org.example.ui.project.chart.freeChart;

import lombok.Data;
import org.apache.commons.collections4.ListUtils;
import org.example.pojo.data.CanMessage;
import org.example.pojo.dbc.DbcSignal;
import org.example.utils.StringUtil;
import org.example.utils.chart.TimeFormater;
import org.example.utils.db.DatabaseManager;
import org.example.utils.db.ExplanCanData;
import org.example.utils.web.ChartDataCacher;
import org.example.utils.web.DateItem;
import org.jfree.data.time.Millisecond;
import org.jfree.data.time.TimeSeries;

import java.util.Date;
import java.util.List;

/**
 * 信号值提取器, 用于从CanMessage中提取信号值
 */

@Data
public class SignalValueExtractor {
    private DbcSignal signal;

    private List<CanMessage> messages;

    private TimeSeries dataSeries;

    private Long  maxCounter;

    private double maxTime;

    private Long projectId;

    private ProjectMessagePicker projectMessagePicker;

    // 第一个步
    public void init(Long projectId,DbcSignal signal){
        this.signal = signal;
        this.projectId = projectId;
        dataSeries = new TimeSeries(signal.getName());
    }

    private double extractForValue(CanMessage message){
        return ExplanCanData.extractSignalValue(ExplanCanData.hexStringToByteArray(message.getData()), signal);
    }

    // 压入数据
    private void addToDataSeries(CanMessage message){
        double value = extractForValue(message);
        DateItem date = TimeFormater.formatTimeToDate(message.getTime());
        Millisecond mt = new Millisecond(date.getMillisecond(), date.getSecond(),
                date.getMinute(), date.getHour(),
                date.getDay(), date.getMonth(),
                date.getYear());
        dataSeries.addOrUpdate(mt, value);
        maxCounter = message.getIndexCounter();
        String time = message.getTime();
        maxTime = Double.parseDouble(time);
    }

    public void cleanDataSeries(){
        dataSeries.clear();
    }

    public void cleanDataAndIndex(){
        cleanDataSeries();
        maxCounter = 0L;
        maxTime = 0.0;
    }

    // 从数据库获取数据时使用这个方法
    public TimeSeries getDataFromDb(Double maxTimePoint){

        List<CanMessage> messageList = getMessageFromProject(projectId,
                StringUtil.getCanId(signal.getMessageId()),
                maxTimePoint);
        for(CanMessage message : messageList){
            addToDataSeries(message);
        }

        return dataSeries;
    }

    // 从缓存中取数据
    public TimeSeries getDataFromCatcher(){
        List<CanMessage> messageList = ChartDataCacher.getInstance().getData(projectId, StringUtil.getCanId(signal.getMessageId()));
        for(CanMessage message : messageList){
            addToDataSeries(message);
        }
        return dataSeries;
    }


    public List<CanMessage> getMessageFromProject(Long projectId,String CanMessageId,Double  maxTimePoint){
        if(projectMessagePicker == null){
            projectMessagePicker = new ProjectMessagePicker();
        }
        return projectMessagePicker.pickMessages(projectId, CanMessageId, maxTimePoint);
    }


    public void cleanDataFromCatch() {
        ChartDataCacher.getInstance().cleanQueue(projectId,StringUtil.getCanId(signal.getMessageId()));
    }
}
