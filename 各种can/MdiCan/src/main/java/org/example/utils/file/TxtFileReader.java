package org.example.utils.file;

import java.io.*;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;

/**
 * Txt文件读取工具类
 * 提供多种方式读取Txt文件内容
 */
public class TxtFileReader {

    /**
     * 读取Txt文件内容，返回字符串
     *
     * @param file 要读取的Txt文件
     * @return 文件内容字符串，如果读取失败返回null
     */
    public static String readFileToString(File file) {
        if (file == null || !file.exists() || !file.isFile()) {
            return null;
        }

        StringBuilder content = new StringBuilder();
        try (BufferedReader reader = new BufferedReader(
                new InputStreamReader(new FileInputStream(file), StandardCharsets.UTF_8))) {

            String line;
            // 读取每一行并添加到字符串构建器
            while ((line = reader.readLine()) != null) {
                content.append(line).append(System.lineSeparator());
            }

            // 移除最后一个换行符
            if (content.length() > 0) {
                content.deleteCharAt(content.length() - 1);
            }

            return content.toString();
        } catch (IOException e) {
            e.printStackTrace();
            return null;
        }
    }

    /**
     * 按行读取Txt文件内容，返回字符串列表
     *
     * @param file 要读取的Txt文件
     * @return 每行内容组成的列表，如果读取失败返回null
     */
    public static List<String> readFileToList(File file) {
        if (file == null || !file.exists() || !file.isFile()) {
            return null;
        }

        List<String> lines = new ArrayList<>();
        try (BufferedReader reader = new BufferedReader(
                new InputStreamReader(new FileInputStream(file), StandardCharsets.UTF_8))) {

            String line;
            // 读取每一行并添加到列表
            while ((line = reader.readLine()) != null) {
                lines.add(line);
            }

            return lines;
        } catch (IOException e) {
            e.printStackTrace();
            return null;
        }
    }

    /**
     * 读取大Txt文件内容，通过回调处理每一行
     * 适合处理大型文本文件，避免内存占用过高
     *
     * @param file 要读取的Txt文件
     * @param lineProcessor 行处理器，用于处理每一行内容
     * @return 读取是否成功
     */
    public static boolean readLargeFile(File file, LineProcessor lineProcessor) {
        if (file == null || !file.exists() || !file.isFile() || lineProcessor == null) {
            return false;
        }

        try (BufferedReader reader = new BufferedReader(
                new InputStreamReader(new FileInputStream(file), StandardCharsets.UTF_8))) {

            String line;
            // 读取每一行并通过回调处理
            while ((line = reader.readLine()) != null) {
                // 如果处理器返回false，则停止读取
                if (!lineProcessor.process(line)) {
                    break;
                }
            }

            return true;
        } catch (IOException e) {
            e.printStackTrace();
            return false;
        }
    }

    /**
     * 行处理器接口，用于处理大文件的每一行
     */
    @FunctionalInterface
    public interface LineProcessor {
        /**
         * 处理一行内容
         *
         * @param line 行内容
         * @return 是否继续处理下一行，true继续，false停止
         */
        boolean process(String line);
    }
}
