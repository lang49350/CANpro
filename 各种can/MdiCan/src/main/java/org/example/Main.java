package org.example;


import org.example.ui.MainFrame;

import org.example.utils.db.InitAllTable;

import javax.swing.*;


public class Main {
    public static void main(String[] args) {
        // 初始化数据库
        InitAllTable.init();
        SwingUtilities.invokeLater(() -> {
            try {
                UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
            } catch (Exception e) {
                throw new RuntimeException(e);
            }
            new MainFrame().setVisible(true);
        });
    }
}
