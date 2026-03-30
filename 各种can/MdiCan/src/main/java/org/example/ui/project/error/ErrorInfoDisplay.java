package org.example.ui.project.error;

import javax.swing.*;
import java.awt.*;

public class ErrorInfoDisplay extends JFrame {
    private JEditorPane errorPane;

    public ErrorInfoDisplay() {
        setTitle("错误信息展示");
        setSize(600, 400);
        setLayout(new BorderLayout());

        // 创建 JEditorPane 并设置为可编辑
        errorPane = new JEditorPane();
        errorPane.setContentType("text/plain");
        errorPane.setEditable(false);

        // 将 JEditorPane 放入滚动面板
        JScrollPane scrollPane = new JScrollPane(errorPane);
        add(scrollPane, BorderLayout.CENTER);

        setVisible(true);
    }

    public void setTitle(String title) {
        super.setTitle(title);
    }

    /**
     * 向 JEditorPane 中添加错误信息
     * @param errorMessage 要添加的错误信息
     */
    public void addErrorInfo(String errorMessage) {
        // 异步更新页面
        SwingUtilities.invokeLater(()->{
            String currentText = errorPane.getText();
            if (!currentText.isEmpty()) {
                // 如果已有信息，先换行
                currentText += "\n";
            }
            currentText += errorMessage;
            errorPane.setText(currentText);
            // 滚动到最底部以显示最新信息
            errorPane.setCaretPosition(currentText.length());
        });

    }


    /**
     * 异步添加错误信息并实时刷新
     */
    public void addErrorInfoEx(String errorMessage) {
        // 使用invokeLater确保UI更新在EDT线程
        SwingUtilities.invokeLater(() -> {
            try {
                // 优化：直接操作Document避免完整文本读取，效率更高
                String lineToAdd = errorMessage + "\n";
                errorPane.getDocument().insertString(
                        errorPane.getDocument().getLength(),
                        lineToAdd,
                        null
                );
                // 滚动到最底部
                errorPane.setCaretPosition(errorPane.getDocument().getLength());
            } catch (Exception e) {
                e.printStackTrace();
            }
        });
    }

    public static void main(String[] args) {
        SwingUtilities.invokeLater(new Runnable() {
            @Override
            public void run() {
                new ErrorInfoDisplay();
            }
        });
    }
}
