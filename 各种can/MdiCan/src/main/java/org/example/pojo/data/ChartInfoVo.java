package org.example.pojo.data;

import lombok.Data;
import org.example.ui.project.chart.freeChart.DraggableTimeChart;
import org.example.ui.project.chart.freeChart.SignalValueExtractor;

@Data
public class ChartInfoVo {
    private String CanMessageId;
    private String signalName;
    private Long maxIndex;
    private Long projectId;
    private DraggableTimeChart chart;
    private SignalValueExtractor extractor;
}
