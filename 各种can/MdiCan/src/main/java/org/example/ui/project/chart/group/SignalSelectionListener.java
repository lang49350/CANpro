package org.example.ui.project.chart.group;

import org.example.pojo.project.SignalInGroup;

import java.util.List;

public interface SignalSelectionListener {
    void onSignalsSelected(List<SignalInGroup> signals);
}
