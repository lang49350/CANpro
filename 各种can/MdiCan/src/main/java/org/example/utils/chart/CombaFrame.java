/*
 * Created by JFormDesigner on Thu Jul 31 09:32:24 CST 2025
 */

package org.example.utils.chart;

import org.example.ui.project.chart.SignalChartDrawer;

import java.awt.*;
import java.awt.event.MouseWheelEvent;
import java.awt.event.MouseWheelListener;
import java.awt.event.WindowEvent;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import javax.swing.*;
import javax.swing.border.*;

/**
 * @author jingq
 * 组合同的绘图页面
 */
public class CombaFrame extends JFrame {
    private Map<String,SignalChartDrawer> chartViewInstances = new HashMap<>();
    private Map<String,JCheckBox> checkBoxes = new HashMap<>();

    private Map<String,JScrollPane> scrollPaneMap = new HashMap<>();
    public CombaFrame() {
        initComponents();
        getChartViewInstance();
        drayCheckBoxes();
        // 最大化
        setExtendedState(JFrame.MAXIMIZED_BOTH);
        // 可调整大小
        setResizable(true);
        dryChartViews();

        // 滚动
        contentScroll.addMouseWheelListener(new MouseWheelListener() {
            @Override
            public void mouseWheelMoved(MouseWheelEvent e) {
                // 获取滚动方向和滚动量
                int wheelRotation = e.getWheelRotation(); // 正数：向下滚动；负数：向上滚动
                int scrollAmount = e.getScrollAmount();   // 滚轮的"步长"（通常为3）

                // 自定义1：控制滚动速度（默认一次滚动3行，这里改为5行）
                int customScrollAmount = 5;

                // 自定义2：水平滚动（默认是垂直滚动，这里按Shift键时改为水平滚动）
                JScrollBar scrollBar = e.isShiftDown()
                        ? contentScroll.getHorizontalScrollBar()  // 水平滚动条
                        : contentScroll.getVerticalScrollBar();    // 垂直滚动条

                // 计算新的滚动位置
                int currentValue = scrollBar.getValue();
                int maxValue = scrollBar.getMaximum() - scrollBar.getVisibleAmount();
                int newValue;

                if (wheelRotation > 0) {
                    // 向下/向右滚动：增加滚动位置
                    newValue = currentValue + customScrollAmount;
                } else {
                    // 向上/向左滚动：减少滚动位置
                    newValue = currentValue - customScrollAmount;
                }

                // 限制滚动范围（避免超出边界）
                newValue = Math.max(scrollBar.getMinimum(), Math.min(newValue, maxValue));
                scrollBar.setValue(newValue);
            }
        });
    }

    private void getChartViewInstance() {
        List<SignalChartDrawer> signalChartDrawers = SignalChartDrawer.instances;
        int counter = 0;
        for (SignalChartDrawer signalChartDrawer : signalChartDrawers) {
            String name = "chart" + counter;
            chartViewInstances.put(name,signalChartDrawer);
            JCheckBox checkBox = new JCheckBox(name);
            checkBox.setSelected(true);
            checkBoxes.put(name,checkBox);
            JScrollPane jScrollPane = new JScrollPane();
            scrollPaneMap.put(name,jScrollPane);
            counter++;
            checkBox.addActionListener(e -> {
                scrollPaneMap.get(name).setVisible(checkBox.isSelected());
            });
        }
    }
    private void drayCheckBoxes() {
        for (Map.Entry<String, JCheckBox> entry : checkBoxes.entrySet()) {
            String name = entry.getKey();
            JCheckBox checkBox = entry.getValue();
            toolBar1.add(checkBox);
        }
    }
    private  void dryChartViews() {
        chartViewInstances.forEach((name, signalChartDrawer) -> {
            if (checkBoxes.get(name).isSelected()) {
                scrollPaneMap.get(name).setViewportView(signalChartDrawer.getContentPane());
                signalChartDrawer.dispose();
                panel.add(scrollPaneMap.get(name));
            }
        });

    }

    // 重写关闭
    @Override
    protected void processWindowEvent(WindowEvent e) {
        if (e.getID() == WindowEvent.WINDOW_CLOSING) {
            // 窗口关闭时触发回调
            dispose();
        }
    }


    private void initComponents() {
        // JFormDesigner - Component initialization - DO NOT MODIFY  //GEN-BEGIN:initComponents  @formatter:off
        dialogPane = new JPanel();
        contentPanel = new JPanel();
        contentScroll = new JScrollPane();
        panel = new JPanel();
        panel2 = new JPanel();
        toolBar1 = new JToolBar();
        hSpacer1 = new JPanel(null);

        //======== this ========
        Container contentPane = getContentPane();
        contentPane.setLayout(new BorderLayout());

        //======== dialogPane ========
        {
            dialogPane.setBorder(new EmptyBorder(12, 12, 12, 12));
            dialogPane.setLayout(new BorderLayout());

            //======== contentPanel ========
            {
                contentPanel.setBorder(new EtchedBorder());
                contentPanel.setLayout(new BoxLayout(contentPanel, BoxLayout.Y_AXIS));

                //======== contentScroll ========
                {

                    //======== panel ========
                    {
                        panel.setLayout(new BoxLayout(panel, BoxLayout.Y_AXIS));
                    }
                    contentScroll.setViewportView(panel);
                }
                contentPanel.add(contentScroll);
            }
            dialogPane.add(contentPanel, BorderLayout.CENTER);

            //======== panel2 ========
            {
                panel2.setLayout(new BoxLayout(panel2, BoxLayout.X_AXIS));

                //======== toolBar1 ========
                {
                    toolBar1.setBorder(LineBorder.createBlackLineBorder());
                }
                panel2.add(toolBar1);
                panel2.add(hSpacer1);
            }
            dialogPane.add(panel2, BorderLayout.NORTH);
        }
        contentPane.add(dialogPane, BorderLayout.CENTER);
        pack();
        setLocationRelativeTo(getOwner());
        // JFormDesigner - End of component initialization  //GEN-END:initComponents  @formatter:on
    }

    // JFormDesigner - Variables declaration - DO NOT MODIFY  //GEN-BEGIN:variables  @formatter:off
    private JPanel dialogPane;
    private JPanel contentPanel;
    private JScrollPane contentScroll;
    private JPanel panel;
    private JPanel panel2;
    private JToolBar toolBar1;
    private JPanel hSpacer1;
    // JFormDesigner - End of variables declaration  //GEN-END:variables  @formatter:on
}
