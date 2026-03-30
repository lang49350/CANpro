package org.example.utils.chart;

import org.apache.commons.collections4.ListUtils;
import org.ehcache.shadow.org.terracotta.statistics.Time;
import org.jfree.data.time.TimeSeries;
import org.jfree.data.time.RegularTimePeriod;
import org.jfree.data.time.TimeSeriesDataItem;

import java.util.*;
import java.util.stream.Collectors;

/**
 * JFreeChart时间序列数据补全工具类
 */
public class TimeSeriesCompletor {
    public static List<RegularTimePeriod> collectTimePoints(List<TimeSeries> timeSeries){
        if(ListUtils.emptyIfNull(timeSeries).isEmpty()){
            return new ArrayList<>();
        }
        Set<RegularTimePeriod> allTimePoints = new TreeSet<>(Comparator.comparing(RegularTimePeriod::getStart));
        for (TimeSeries series : timeSeries) {
            for (int i=0;i< series.getItemCount();i++){
                allTimePoints.add(series.getTimePeriod(i));
            }
        }
        return new ArrayList<>(allTimePoints);
    }

    public static TimeSeries completeTimeSeries(TimeSeries timeSeries, List<RegularTimePeriod> allTimes){
        if(allTimes == null || allTimes.isEmpty() || timeSeries == null){
            return timeSeries;
        }
        TimeSeries completeTimeSeries = new TimeSeries(timeSeries.getKey());
        Number lastValue = null;
        for (RegularTimePeriod time : allTimes) {
            try {
                int index = timeSeries.getIndex(time);
                if(index>0){
                    lastValue = timeSeries.getValue(index);
                    completeTimeSeries.addOrUpdate(time, lastValue);
                }else if (lastValue != null){
                    completeTimeSeries.addOrUpdate(time, lastValue);
                }

            }catch (Exception e){
                e.printStackTrace();
            }
        }
        return completeTimeSeries;
    }
}