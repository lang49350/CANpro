package org.example.utils.proto;

import com.google.protobuf.CodedInputStream;
import org.example.pojo.data.CanMessage;
import org.example.ui.project.error.ErrorInfoDisplay;
import org.example.utils.table.EnhancedTable;

import javax.swing.*;
import java.io.BufferedInputStream;
import java.io.FileInputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicLong;

public class CanProtobufReader {

    // 1. 缓冲大小：32KB（平衡内存占用和磁盘IO效率，SSD可设为64KB）
    private static final int BUFFER_SIZE = 32 * 1024;
    // 2. 单个消息最大限制：1MB（防止异常消息导致OOM，根据实际CAN数据调整）
    private static final int MAX_SINGLE_MESSAGE_SIZE = 1024 * 1024;
    // 3. 进度打印间隔：每处理10万条消息打印一次（减少IO开销）
    private static final long PROGRESS_INTERVAL = 100000;
    // 新增：每批处理的消息数量
    private static final int BATCH_SIZE = 1000;

    private EnhancedTable<CanMessage> table;

    ErrorInfoDisplay errorDialog = new ErrorInfoDisplay();


    public void setTable(EnhancedTable<CanMessage> table) {
        this.table = table;
    }

    public static void main(String[] args) {
        // 替换为你的 500M Protobuf 文件路径
        String largeProtobufPath = "D:\\protoc-32.0-win64\\proto\\output_large.pb";
        CanProtobufReader reader = new CanProtobufReader();
        // 调用流式读取方法
        reader.readLargeCanProtobuf(largeProtobufPath,
                protoMsg -> {
                    CanMessage canEntity = new CanMessage();
                    // 从 Protobuf 消息中提取字段（对应生成文件的 getXxx() 方法）
                    canEntity.setCanId(protoMsg.getCanId());   // 获取 CAN ID（16进制字符串）
                    canEntity.setData(protoMsg.getData());     // 获取数据（16进制字符串）
                    canEntity.setTime(protoMsg.getTime());     // 获取时间戳字符串

                    // 这里添加你的业务逻辑（如写入数据库、过滤数据等）
                    // 示例：打印前10条消息验证
                    if (count.get() <= 10) {
                        System.out.printf("验证消息：CAN ID=%s, 时间=%s, 数据=%s%n",
                                canEntity.getCanId(), canEntity.getTime(), canEntity.getData());
                    }
                });
    }

    // 原子计数器：统计总消息数（线程安全）
    public static final AtomicLong count = new AtomicLong(0);
    public static final AtomicLong errCount = new AtomicLong(0);

    /**
     * 流式读取大体积 CAN Protobuf 文件
     * @param protobufPath Protobuf 文件路径
     * @param processor 消息处理器（解耦读取和业务逻辑）
     */
    public  void readLargeCanProtobuf(String protobufPath, CanMessageProcessor processor) {
        showInformation("开始读取文件：" + protobufPath);
        long startTime = System.currentTimeMillis();
        List<CanMessage> batchData = new ArrayList<>(BATCH_SIZE);

        try (
                // 1. 缓冲输入流：减少磁盘IO次数（核心优化点1）
                BufferedInputStream bis = new BufferedInputStream(
                        new FileInputStream(protobufPath), BUFFER_SIZE);
        ) {
            // 2. Protobuf 流式解析器：逐条读取消息（核心优化点2）
            CodedInputStream cis = CodedInputStream.newInstance(bis);
            // 设置单个消息最大大小，防止恶意数据/OOM
            cis.setSizeLimit(MAX_SINGLE_MESSAGE_SIZE);

            // 3. 循环读取消息列表中的每条消息（不加载全量数据）
            while (!cis.isAtEnd()) {
                cis.resetSizeCounter();
                // 读取字段标记（tag = 字段号 << 3 | 字段类型）
                int tag = cis.readTag();

                // 匹配 "messages" 字段（生成文件中该字段的 tag 是 10）
                // 10 = 字段号1 << 3 | 类型2（LengthDelimited，变长消息类型）
                if (tag == 10) {
                    // 读取单条 CAN 消息（流式解析，仅占用单条消息内存）
                    org.example.utils.proto.CanMessage.CanMessageProto protoMsg =
                            org.example.utils.proto.CanMessage.CanMessageProto.parseFrom(cis.readBytes());

                    CanMessage canEntity = new CanMessage();
                    canEntity.setCanId(protoMsg.getCanId());
                    canEntity.setData(protoMsg.getData());
                    canEntity.setTime(protoMsg.getTime());
                    canEntity.setIndexCounter(count.get());
                    batchData.add(canEntity);

                    // 处理消息（交给业务处理器）
//                    processor.process(protoMsg);

                    // 统计消息数并打印进度
                    long currentCount = count.incrementAndGet();
//                    if (currentCount % PROGRESS_INTERVAL == 0) {
//                        long costSeconds = (System.currentTimeMillis() - startTime) ;
//                        System.out.printf("已处理 %d 条消息，耗时 %d 毫秒，平均每毫秒处理 %.2f 条%n",
//                                currentCount, costSeconds, (double) currentCount / costSeconds);
//                    }

                    // 达到批次大小，处理并清空列表
                    if (batchData.size() >= BATCH_SIZE) {
                        // 这里添加持久化逻辑，例如写入数据库
                        // 示例：假设存在一个 saveBatch 方法
                        saveBatch(batchData);
                        batchData.clear();
                        try {
                            Thread.sleep(10);
                        }catch (InterruptedException e){
                            e.printStackTrace();
                        }
                    }
                } else {
                    // 跳过未知字段（避免解析失败，兼容未来可能的字段扩展）
                    JOptionPane.showMessageDialog(null, "未知字段：" + tag);
                    System.out.println("Skipping unknown field: " + tag);
                    cis.skipField(tag);
                }
            }

            // 处理最后一批数据
            if (!batchData.isEmpty()) {
                saveBatch(batchData);
                batchData.clear();
            }

            // 4. 处理完成，打印统计信息
            long totalCost = (System.currentTimeMillis() - startTime) / 1000;
            System.out.printf("%n处理完成！总消息数：%d，总耗时：%d 秒，平均每秒处理 %.2f 条%n",
                    count.get(), totalCost, (double) count.get() / totalCost);
            showInformation("处理完成！总消息数："
                    + count.get() + "，总耗时："
                    + totalCost
                    + " 秒，平均每秒处理 "
                    + (double) count.get() / totalCost
                    + " 条。"
                    + "\n"
                    + "表格数据更新仍在继续请，等待表格数据更新完成"
            );
            // 新增：等待表格处理完所有数据
            try {
                if (table != null) {
                    table.waitForTasks();
                    showInformation("**********************表格数据更新完成*********************");
                }
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        } catch (IOException e) {
            System.err.println("读取 Protobuf 文件失败：" + e.getMessage());
            e.printStackTrace();
        }
    }

    private void showInformation(String s) {
        // 新线程中启动ErrorDialog
        SwingUtilities.invokeLater(() -> {
            errorDialog.setTitle("信息");
            errorDialog.addErrorInfoEx(s);
            errorDialog.setVisible(true);
        });
    }

    /**
     * 批量保存 CanMessage 到持久化存储
     * @param batchData 待保存的 CanMessage 列表
     */
    private  void saveBatch(List<CanMessage> batchData) {
        // 实现具体的持久化逻辑，例如使用 MyBatis 批量插入数据库
        // 这里只是示例，需要根据实际情况实现
        // 建立batchData 的拷贝
        List<CanMessage> batchDataCopy = new ArrayList<>(batchData);
        System.out.printf("批量保存 %d 条消息%n", batchDataCopy.size());
        table.appendBatchNoSort(batchDataCopy);
    }

    /**
     * 消息处理器接口（解耦读取逻辑和业务逻辑）
     */
    @FunctionalInterface
    public interface CanMessageProcessor {
        void process(org.example.utils.proto.CanMessage.CanMessageProto protoMsg);
    }
}