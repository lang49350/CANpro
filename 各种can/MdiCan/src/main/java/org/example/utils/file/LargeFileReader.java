package org.example.utils.file;

import lombok.Data;
import org.apache.ibatis.session.SqlSession;
import org.example.mapper.CanMessageMapper;
import org.example.pojo.data.CanMessage;
import org.example.utils.db.DatabaseManager;
import org.example.utils.db.TableNameContextHolder;

import javax.swing.*;
import java.awt.*;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.util.List;

import java.util.ArrayList;
import java.util.concurrent.atomic.AtomicInteger;
@Data
public class LargeFileReader extends JFrame {
    private final JProgressBar progressBar;
    private final JTextArea logArea;
    private FileReadingWorker worker;
    // 批次大小（可根据数据库性能调整）
    private static final int BATCH_SIZE = 5000;
    // 数据库连接配置（实际项目中建议放在配置文件）
    private  Long projectId;

    public LargeFileReader() {
        setTitle("TXT文件批量导入数据库");
        setSize(800, 600);
        setDefaultCloseOperation(JFrame.DISPOSE_ON_CLOSE);
        setLocationRelativeTo(null);

        // 初始化UI组件
        progressBar = new JProgressBar(0, 100);
        progressBar.setStringPainted(true);
        logArea = new JTextArea();
        logArea.setEditable(false);
        JButton cancelButton = new JButton("取消");
        cancelButton.setEnabled(false);

        // 布局
        JPanel panel = new JPanel(new BorderLayout());
        panel.add(new JScrollPane(logArea), BorderLayout.CENTER);

        JPanel controlPanel = new JPanel(new BorderLayout(10, 10));
        controlPanel.add(progressBar, BorderLayout.CENTER);
        controlPanel.add(cancelButton, BorderLayout.EAST);
        panel.add(controlPanel, BorderLayout.SOUTH);

        add(panel);

        // 绑定事件
        cancelButton.addActionListener(e -> {
            if (worker != null && !worker.isDone()) {
                worker.cancel(true);
                logArea.append("操作已取消\n");
                cancelButton.setEnabled(false);
            }
        });
        createMenuBar();
    }

    private void createMenuBar() {
        JMenuBar menuBar = new JMenuBar();
        JMenu fileMenu = new JMenu("文件");
        JMenuItem openItem = new JMenuItem("选择TXT文件并导入");

        openItem.addActionListener(e -> {
            JFileChooser chooser = new JFileChooser();
            chooser.setFileFilter(new javax.swing.filechooser.FileFilter() {
                @Override
                public boolean accept(File f) {
                    return f.isDirectory() || f.getName().toLowerCase().endsWith(".txt");
                }
                @Override
                public String getDescription() {
                    return "文本文件 (*.txt)";
                }
            });

            int result = chooser.showOpenDialog(this);
            if (result == JFileChooser.APPROVE_OPTION) {
                File file = chooser.getSelectedFile();
                startImport(file);
            }
        });

        fileMenu.add(openItem);
        menuBar.add(fileMenu);
        setJMenuBar(menuBar);
    }

    private void startImport(File file) {
        if (worker != null && !worker.isDone()) {
            worker.cancel(true);
        }

        progressBar.setValue(0);
        logArea.setText("开始导入文件: " + file.getAbsolutePath() + "\n");
        logArea.append("批次大小: " + BATCH_SIZE + " 条/批\n");
//        ((JButton) getRootPane().getContentPane().getComponent(0)
//                .getComponent(1).getComponent(1)).setEnabled(true);

        worker = new FileReadingWorker(file);
        worker.execute();
    }

    // 后台处理任务：读取文件+批量入库
    private class FileReadingWorker extends SwingWorker<Void, String> {
        private final File file;
        private long totalLines = 0;
        private long readLines = 0;
        private long successCount = 0;
        // 缓存当前批次的数据
        private List<String> batchData = new ArrayList<>(BATCH_SIZE);

        public FileReadingWorker(File file) {
            this.file = file;
        }

        @Override
        protected Void doInBackground() throws Exception {
            try {
                countTotalLines();
                if (totalLines == 0) {
                    publish("文件为空，无需导入");
                    return null;
                }

                readAndImportFile();
                // 处理最后一批不足BATCH_SIZE的数据
                if (!batchData.isEmpty()) {
                    int inserted = batchInsertToDB(batchData);
                    successCount += inserted;
                    publish("最后一批插入完成，新增 " + inserted + " 条数据");
                }
                publish("导入完成！总读取: " + readLines + " 行，成功插入: " + successCount + " 条");
            } catch (Exception e) {
                publish("导入失败: " + e.getMessage());
                e.printStackTrace();
            }
            return null;
        }

        // 计算总行数
        private void countTotalLines() throws IOException {
            publish("正在计算文件总行数...");
            try (BufferedReader reader = new BufferedReader(
                    new InputStreamReader(new FileInputStream(file), StandardCharsets.UTF_8))) {
                while (reader.readLine() != null && !isCancelled()) {
                    totalLines++;
                }
            }
            publish("文件总行数: " + totalLines + " 行");
        }

        // 读取文件并批量导入
        private void readAndImportFile() throws IOException {
            try (BufferedReader reader = new BufferedReader(
                    new InputStreamReader(new FileInputStream(file), StandardCharsets.UTF_8))) {

                String line;
                while ((line = reader.readLine()) != null && !isCancelled()) {
                    readLines++;
                    // 过滤空行（根据需求调整）
                    if (line.trim().isEmpty()) continue;

                    // 添加到批次缓存
                    batchData.add(line);

                    // 达到批次大小则执行批量插入
                    if (batchData.size() >= BATCH_SIZE) {
                        int inserted = batchInsertToDB(batchData);
                        successCount += inserted;
                        // 清空缓存，准备下一批
                        batchData.clear();
                        // 更新进度
                        int progress = (int) ((readLines * 100.0) / totalLines);
                        setProgress(progress);
                        publish("已处理 " + readLines + " 行 (" + progress + "%)，累计插入: " + successCount + " 条");
                    }
                }
            }
        }

        // 核心：批量插入数据库
        private int batchInsertToDB(List<String> dataList) {
            List<CanMessage> canMessages = CanMessage.toCanMessageList(dataList);
            if(canMessages.isEmpty()) {
                return 0;
            }
            AtomicInteger inserted = new AtomicInteger();
            DatabaseManager databaseManager = new DatabaseManager();
            databaseManager.execute(CanMessage.class, (SqlSession session) -> {
                CanMessageMapper mapper = session.getMapper(CanMessageMapper.class);
                TableNameContextHolder.setSuffix("can_message","_p_" + projectId);
                inserted.set(mapper.insertBatch(canMessages));
                TableNameContextHolder.clearSuffix("can_message");
                return inserted.get();
            });

            return inserted.get();
        }

        @Override
        protected void process(List<String> chunks) {
            for (String msg : chunks) {
                logArea.append(msg + "\n");
                logArea.setCaretPosition(logArea.getDocument().getLength());
            }
        }

        @Override
        protected void done() {
            progressBar.setValue(isCancelled() ? 0 : 100);
//            ((JButton) getRootPane().getContentPane().getComponent(0)
//                    .getComponent(1).getComponent(1)).setEnabled(false);
        }
    }

    public static void main(String[] args) {

        SwingUtilities.invokeLater(() -> new LargeFileReader().setVisible(true));
    }
}

