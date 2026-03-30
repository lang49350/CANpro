package org.example.utils.web;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

//单例模式
// 记录接收数据的总数，按项目记录
public class ReceivedDataReCorder {
    private static ReceivedDataReCorder instance;
    // 项目Id，记录数
    private static Map<Long, Long> projectReceivedDataCountMap = new java.util.HashMap<>();
    private static Map<Long, List<Double>> projectReceivedSpeedMap = new java.util.HashMap<>();
    //  私有构造函数
    private ReceivedDataReCorder() {
        projectReceivedDataCountMap = new HashMap<>();
        projectReceivedSpeedMap = new HashMap<>();
    }
    // 单例模式，获取实例
    public static ReceivedDataReCorder getInstance() {
        if (instance == null) {
            instance = new ReceivedDataReCorder();
        }
        return instance;
    }
    // 记录项目接收数据的总数
    public void setProjectReceivedDataCountMap(Long projectId, long count) {
        projectReceivedDataCountMap.put(projectId, count);
    }
    // 获取项目接收数据的总数
    public long getProjectReceivedDataCount(Long projectId) {
        return projectReceivedDataCountMap.getOrDefault(projectId, 0L);
    }
    // 清除某个项目的接收数据总数为0
    public void clearProjectReceivedDataCount(Long projectId) {
        projectReceivedDataCountMap.put(projectId, 0L);
    }
    // 某个项目的接收数据总数加 n
    public void addProjectReceivedDataCount(Long projectId, long n) {
        projectReceivedDataCountMap.put(projectId, getProjectReceivedDataCount(projectId) + n);
    }

    public void setProjectReceivedSpeedMap(Long projectId, double speed) {
        List<Double> doubles = projectReceivedSpeedMap.get(projectId);
        if(doubles == null){
            projectReceivedSpeedMap.put(projectId, new java.util.ArrayList<>());
        }
        projectReceivedSpeedMap.get(projectId).add(speed);
    }
    // 获取项目接收数据的速率

    public double getEveSpeed(Long projectId) {
        List<Double> doubles = projectReceivedSpeedMap.get(projectId);
        if(doubles == null || doubles.isEmpty()){
            return 0.0;
        }
        return doubles.stream().mapToDouble(Double::doubleValue).average().orElse(0.0);
    }

    // 获取最大速率
    public double getMaxSpeed(Long projectId) {
        List<Double> doubles = projectReceivedSpeedMap.get(projectId);
        if(doubles == null || doubles.isEmpty()){
            return 0.0;
        }
        return doubles.stream().mapToDouble(Double::doubleValue).max().orElse(0.0);
    }
    public void clearProjectReceivedSpeedMap(Long projectId) {
        projectReceivedSpeedMap.put(projectId, new java.util.ArrayList<>());
    }

}
