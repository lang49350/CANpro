package org.example.ui.project.waitting;

import javax.swing.*;
import java.awt.*;

public class WaittingView {
    private JFrame mainFrame;
    private JDialog progressDialog;

    // 添加构造函数，接收主窗口引用
    public WaittingView(JFrame mainFrame) {
        this.mainFrame = mainFrame;
    }

    public void show() {
        // 如果对话框已存在，先关闭它
        if (progressDialog != null && progressDialog.isVisible()) {
            progressDialog.dispose();
        }

        // 创建新的对话框
        progressDialog = new JDialog(mainFrame, "处理中");
        progressDialog.setSize(300, 120);
        progressDialog.setLocationRelativeTo(mainFrame);
        progressDialog.setResizable(false);

        JLabel progressLabel = new JLabel("正在处理，请稍候...");
        progressLabel.setHorizontalAlignment(JLabel.CENTER);

        // 使用更合适的布局
        JPanel panel = new JPanel(new BorderLayout());
        panel.add(progressLabel, BorderLayout.CENTER);
        progressDialog.add(panel);

        // 显示对话框（此方法会阻塞，直到对话框关闭）
        progressDialog.setVisible(true);
    }

    public void close() {
        // 检查对话框是否存在且可见
        if (progressDialog != null && progressDialog.isVisible()) {
            progressDialog.dispose();
        }
    }

    public static void main(String[] args) {
        // 在事件调度线程中创建GUI
        SwingUtilities.invokeLater(() -> {
            // 创建主窗口
            JFrame frame = new JFrame("测试等待视图");
            frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
            frame.setSize(400, 300);
            frame.setLocationRelativeTo(null);

            JButton showButton = new JButton("显示等待视图");
            showButton.addActionListener(e -> {
                // 创建并显示等待视图
                WaittingView waitView = new WaittingView(frame);

                // 在单独线程中执行耗时操作
                new Thread(() -> {
                    // 显示等待视图
                    SwingUtilities.invokeLater(waitView::show);

                    try {
                        // 模拟耗时操作
                        Thread.sleep(3000);
                    } catch (InterruptedException ex) {
                        ex.printStackTrace();
                    }

                    // 关闭等待视图
                    SwingUtilities.invokeLater(waitView::close);
                }).start();
            });

            frame.add(showButton, BorderLayout.CENTER);
            frame.setVisible(true);
        });
    }
}