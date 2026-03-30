package org.example.utils.file;

import javax.swing.JFileChooser;
import javax.swing.filechooser.FileFilter;
import java.io.File;

/**
 * Swing中选择Txt文件的工具类
 * 提供标准的Txt文件选择方法，可直接调用
 */
public class FileChooserUtils {

    /**
     * 选择单个Txt文件
     *
     * @return 选中的Txt文件，如果未选择则返回null
     */
    public static File chooseTxtFile() {
        // 创建文件选择器
        JFileChooser fileChooser = new JFileChooser();

        // 设置文件选择模式为文件选择
        fileChooser.setFileSelectionMode(JFileChooser.FILES_ONLY);

        // 设置当前目录为用户主目录
        fileChooser.setCurrentDirectory(new File(System.getProperty("user.home")));

        // 添加Txt文件过滤器
        fileChooser.setFileFilter(new FileFilter() {
            @Override
            public boolean accept(File file) {
                // 接受目录和txt文件
                return file.isDirectory() || file.getName().toLowerCase().endsWith(".txt");
            }

            @Override
            public String getDescription() {
                return "文本文件 (*.txt)";
            }
        });

        // 显示文件选择对话框，返回用户操作结果
        int result = fileChooser.showOpenDialog(null);

        // 如果用户选择了文件，返回选中的文件，否则返回null
        return result == JFileChooser.APPROVE_OPTION ? fileChooser.getSelectedFile() : null;
    }

    /**
     * 选择多个Txt文件
     *
     * @return 选中的Txt文件数组，如果未选择则返回空数组
     */
    public static File[] chooseMultipleTxtFiles() {
        // 创建文件选择器
        JFileChooser fileChooser = new JFileChooser();

        // 设置文件选择模式为文件选择
        fileChooser.setFileSelectionMode(JFileChooser.FILES_ONLY);

        // 允许选择多个文件
        fileChooser.setMultiSelectionEnabled(true);

        // 设置当前目录为用户主目录
        fileChooser.setCurrentDirectory(new File(System.getProperty("user.home")));

        // 添加Txt文件过滤器
        fileChooser.setFileFilter(new FileFilter() {
            @Override
            public boolean accept(File file) {
                return file.isDirectory() || file.getName().toLowerCase().endsWith(".txt");
            }

            @Override
            public String getDescription() {
                return "文本文件 (*.txt)";
            }
        });

        // 显示文件选择对话框
        int result = fileChooser.showOpenDialog(null);

        // 返回选中的文件数组或空数组
        return result == JFileChooser.APPROVE_OPTION ? fileChooser.getSelectedFiles() : new File[0];
    }
}
