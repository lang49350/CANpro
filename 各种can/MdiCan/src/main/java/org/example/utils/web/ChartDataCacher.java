package org.example.utils.web;

import cn.hutool.core.bean.BeanUtil;
import org.apache.commons.collections4.ListUtils;
import org.example.pojo.data.CanMessage;
import org.example.utils.StringUtil;

import java.util.*;
import java.util.concurrent.ConcurrentLinkedQueue;

/**
 * 用于缓存当前在绘制的曲线的数据
 * 单实例
 * 数据存储在chartData ,结构 projectId , CanMessageId,queue of Can message
 */
public class ChartDataCacher {
    private static ChartDataCacher instance;

    private static Map<Long,Map<String, ConcurrentLinkedQueue<CanMessage>>> dataMap;

    // to fast read data use Map to store ,long of inner map is useless
    private static Map<Long,Map<String, Long>> currentProjectCharts;

    private ChartDataCacher(){
        dataMap = new HashMap<>();
        currentProjectCharts = new HashMap<>();
    }

    public static ChartDataCacher getInstance(){
        if(instance == null){
            instance = new ChartDataCacher();
        }
        return instance;
    }

    public boolean isMessageNeeded(Long projectId, String canMessageId){
        if(currentProjectCharts.get(projectId) == null){
            return false;
        }
        if(currentProjectCharts.get(projectId).get(canMessageId) == null){
            return false;
        }
        return currentProjectCharts.get(projectId).get(canMessageId) !=null;
    }
    public void  setChartToCurrent(Long projectId, String canMessageId){
        Map<String, Long> projectMap = currentProjectCharts.get(projectId);
        if(projectMap == null){
            projectMap = new HashMap<>();
            currentProjectCharts.put(projectId, projectMap);
        }

        projectMap.put(StringUtil.getCanId(canMessageId), 0L);
    }
    public void addData(Long projectId, String canMessageId, CanMessage data){
        Map<String, ConcurrentLinkedQueue<CanMessage>> projectMap = dataMap.get(projectId);
        if (projectMap == null){
            projectMap = new HashMap<>();
            dataMap.put(projectId, projectMap);
        }
        if(projectMap.get(canMessageId) == null){
            projectMap.put(canMessageId, new ConcurrentLinkedQueue<>());
        }
        dataMap.get(projectId).get(canMessageId).add(data);

    }

    public List<CanMessage> getData(Long projectId, String canMessageId){
        Map<String, ConcurrentLinkedQueue<CanMessage>> projectMap = dataMap.get(projectId);
        if(projectMap == null){
            return new ArrayList<>();
        }
        if(projectMap.get(canMessageId) == null){
            return new ArrayList<>();
        }
        List<CanMessage> result = new ArrayList<>();
//        while (!projectMap.get(canMessageId).isEmpty()){
//            CanMessage canMessage = new CanMessage();
//            BeanUtil.copyProperties(projectMap.get(canMessageId).peek(),canMessage);
//            result.add(canMessage);
////            result.add(projectMap.get(canMessageId).poll());
//        }
        for(int i=0;i<projectMap.get(canMessageId).size();i++){
            CanMessage canMessage = new CanMessage();
            BeanUtil.copyProperties(projectMap.get(canMessageId).peek(),canMessage);
            result.add(canMessage);
        }
        return result;
    }

    public void cleanQueue(Long projectId,String canMessageId){
        Map<String, ConcurrentLinkedQueue<CanMessage>> projectMap = dataMap.get(projectId);
        if(projectMap == null){
            return;
        }
        if(projectMap.get(canMessageId) == null){
            return;
        }
        projectMap.get(canMessageId).clear();
    }
}
