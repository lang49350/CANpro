package org.example.ui.project.chart.freeChart;

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;

/**
 * 可拖动和调整大小的面板
 */
public class DraggableResizablePanel extends JPanel {
    // 拖动相关变量
    private boolean isDragging = false;
    private int dragStartX, dragStartY;
    private int panelStartX, panelStartY;

    // 调整大小相关变量
    private boolean isResizing = false;
    private int resizeStartX, resizeStartY;
    private Dimension startSize;
    private static final int RESIZE_BORDER = 8; // 边界区域大小

    public DraggableResizablePanel(String title, Color color) {
        setBorder(BorderFactory.createTitledBorder(title));
        setBackground(color);
        setPreferredSize(new Dimension(200, 150));

        // 添加鼠标监听器处理拖动和调整大小
        addMouseListener(new MouseAdapter() {
            @Override
            public void mousePressed(MouseEvent e) {
                // 检查是否在边界区域（调整大小）
                if (isInResizeArea(e)) {
                    isResizing = true;
                    resizeStartX = e.getXOnScreen();
                    resizeStartY = e.getYOnScreen();
                    startSize = getSize();
                    setCursor(Cursor.getPredefinedCursor(Cursor.SE_RESIZE_CURSOR));
                } else {
                    // 否则处理拖动
                    isDragging = true;
                    dragStartX = e.getXOnScreen();
                    dragStartY = e.getYOnScreen();
                    panelStartX = getX();
                    panelStartY = getY();
                    setCursor(Cursor.getPredefinedCursor(Cursor.MOVE_CURSOR));
                }
            }

            @Override
            public void mouseReleased(MouseEvent e) {
                isDragging = false;
                isResizing = false;
                setCursor(Cursor.getDefaultCursor());
            }
        });

        addMouseMotionListener(new MouseMotionAdapter() {
            @Override
            public void mouseDragged(MouseEvent e) {
                if (isResizing) {
                    // 处理调整大小
                    int dx = e.getXOnScreen() - resizeStartX;
                    int dy = e.getYOnScreen() - resizeStartY;

                    int newWidth = Math.max(100, startSize.width + dx); // 最小宽度
                    int newHeight = Math.max(80, startSize.height + dy); // 最小高度

                    setSize(newWidth, newHeight);
                    revalidate();
                    repaint();
                } else if (isDragging) {
                    // 处理拖动
                    int dx = e.getXOnScreen() - dragStartX;
                    int dy = e.getYOnScreen() - dragStartY;

                    int newX = panelStartX + dx;
                    int newY = panelStartY + dy;

                    // 确保面板不会移出父容器
                    Container parent = getParent();
                    if (parent != null) {
                        newX = Math.max(0, Math.min(newX, parent.getWidth() - getWidth()));
                        newY = Math.max(0, Math.min(newY, parent.getHeight() - getHeight()));
                    }

                    setLocation(newX, newY);
                    repaint();
                }
            }

            @Override
            public void mouseMoved(MouseEvent e) {
                // 鼠标移动时改变光标样式
                if (isInResizeArea(e)) {
                    setCursor(Cursor.getPredefinedCursor(Cursor.SE_RESIZE_CURSOR));
                } else {
                    setCursor(Cursor.getDefaultCursor());
                }
            }
        });
    }

    // 检查鼠标是否在可调整大小的边界区域
    private boolean isInResizeArea(MouseEvent e) {
        Rectangle bounds = getBounds();
        return e.getX() >= bounds.width - RESIZE_BORDER &&
                e.getY() >= bounds.height - RESIZE_BORDER;
    }
}
