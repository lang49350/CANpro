package org.example.ui.project.chart.bitView;

import com.baomidou.mybatisplus.core.conditions.query.QueryWrapper;
import org.example.mapper.CProjectMapper;
import org.example.mapper.DbcSignalMapper;
import org.example.pojo.data.SendMessage;
import org.example.pojo.dbc.DbcSignal;
import org.example.pojo.project.CProject;
import org.example.utils.chart.DistinctRandomColorGenerator;
import org.example.utils.db.DatabaseManager;

import javax.swing.*;
import java.awt.*;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.util.ArrayList;
import java.util.List;

public class CanFdBitViewer extends JFrame {
    private SendMessage currentMessage;
    private int[][] canData = new int[64][8]; // 存储解析后的位数据，64字节×8位
    private List<Signal> signals = new ArrayList<>(); // 信号定义列表

    private int byteNum;

    // 信号定义内部类
    private static class Signal {
        int startBit;   // 全局起始位（直接对应DbcSignal的startBit）
        int length;     // 信号长度（位）
        String name;    // 信号名称
        Color color;    // 背景色

        public Signal(int startBit, int length, String name, Color color) {
            this.startBit = startBit;
            this.length = length;
            this.name = name;
            this.color = color;
        }
    }

    public CanFdBitViewer(SendMessage sendMessage) {
        super("CAN FD 数据位图显示");
        setDefaultCloseOperation(JFrame.DISPOSE_ON_CLOSE);
        setLayout(new BorderLayout());
        setSize(800, 500);
        currentMessage = sendMessage;
        // 初始化信号定义（实际应用中应从数据库或DBC文件加载）
        initSignals();

        // 创建UI组件
        createUI();

        // 添加窗口关闭监听
        addWindowListener(new WindowAdapter() {
            @Override
            public void windowClosing(WindowEvent e) {
                dispose();
            }
        });
    }

    // 初始化示例信号（实际应根据projectId加载对应信号定义）
    private void initSignals() {
        // 根据消息ID和绑定的项目ID加载信号定义
        DatabaseManager databaseManager = new DatabaseManager();
        CProject cProject = databaseManager.execute(CProject.class, session -> {
            return session.getMapper(CProjectMapper.class).selectById(currentMessage.getProjectId());
        });
        String formattedHex = String.format("0x%08X",
                Integer.parseInt(currentMessage.getCanId().replace("0X","")
                        .replace("0x",""), 16));
        List<DbcSignal> mySignals = databaseManager.execute(DbcSignal.class, (session) -> {
            DbcSignalMapper mapper = session.getMapper(DbcSignalMapper.class);
            QueryWrapper<DbcSignal> wrapper = new QueryWrapper<>();
            wrapper.lambda().eq(DbcSignal::getMessageId, formattedHex)
                    .eq(DbcSignal::getDbcMetadataId, cProject.getDbcMetadataId());
            return mapper.selectList(wrapper);
        });
        DistinctRandomColorGenerator colorGenerator = new DistinctRandomColorGenerator();
        for (DbcSignal s : mySignals) {
            Signal signal = new Signal(Integer.parseInt(s.getStartBit()),
                    Integer.parseInt(s.getLength()), s.getName(),colorGenerator.getNextColor());
            signals.add( signal);
        }
    }

    // 创建UI界面
    private void createUI() {
        // 顶部信息栏
        JPanel infoPanel = new JPanel(new GridLayout(1, 4, 10, 5));
        infoPanel.setBorder(BorderFactory.createTitledBorder("消息信息"));
        JLabel canIdLabel = new JLabel("CAN ID: " + currentMessage.getCanId());
        JLabel dataLabel = new JLabel("DATA: " + currentMessage.getData());
        infoPanel.add(canIdLabel);
        infoPanel.add(dataLabel);
        add(infoPanel, BorderLayout.NORTH);


        // 中间位图显示区
        JPanel bitPanel = new JPanel(new GridBagLayout());
        GridBagConstraints gbc = new GridBagConstraints();
        gbc.insets = new Insets(1, 1, 1, 1);
        gbc.anchor = GridBagConstraints.CENTER;
        gbc.fill = GridBagConstraints.BOTH;
        gbc.weightx = 1.0;
        gbc.weighty = 1.0;

        // 添加列标题（位索引）
        for (int col = 0; col < 8; col++) {
            gbc.gridx = col + 1;
            gbc.gridy = 0;
            JLabel label = new JLabel(String.valueOf(col));
            label.setFont(new Font("Arial", Font.BOLD, 10));
            bitPanel.add(label, gbc);
        }

        // 添加行和位单元格
        int maxRows = 0;
        maxRows = ( this.currentMessage!=null && this.currentMessage.getData().length()/2<=8) ?  8 : 64;
        for (int row = 0; row < maxRows; row++) {
            // 行标题（字节索引）
            gbc.gridx = 0;
            gbc.gridy = row + 1;
            JLabel rowLabel = new JLabel("byte " + row);
            rowLabel.setFont(new Font("Arial", Font.BOLD, 10));
            bitPanel.add(rowLabel, gbc);

            // 位单元格
            for (int col = 0; col < 8; col++) {
                gbc.gridx = col + 1;
                gbc.gridy = row + 1;
                BitCell cell = new BitCell(row, col);
                bitPanel.add(cell, gbc);
            }
        }
        // 强制重新布局
        bitPanel.revalidate();
        bitPanel.repaint();

        JScrollPane scrollPane = new JScrollPane(bitPanel);
        add(scrollPane, BorderLayout.CENTER);

        // 底部按钮区
        JPanel buttonPanel = new JPanel();
        JButton refreshButton = new JButton("刷新显示");
        refreshButton.addActionListener(e -> repaint());
        buttonPanel.add(refreshButton);
        add(buttonPanel, BorderLayout.SOUTH);

        setLocationRelativeTo(null);
    }

    // 设置消息并更新显示
    public void setMessage(SendMessage message) {
        this.currentMessage = message;
        if (message != null) {
            parseData(message.getData());
            updateInfoPanel();
            repaint();
        }
    }

    // 解析16进制数据字符串为位数据
    private void parseData(String hexData) {
        // 清除原有数据
        for (int i = 0; i < 64; i++) {
            for (int j = 0; j < 8; j++) {
                canData[i][j] = 0;
            }
        }

        if (hexData == null || hexData.isEmpty()) return;

        // 移除可能的0x前缀
        hexData = hexData.replace("0x", "");

        // 解析每个字节
        for (int i = 0; i < Math.min(64, hexData.length() / 2); i++) {
            int byteValue;
            try {
                // 取两个字符作为一个字节
                String byteStr = hexData.substring(i * 2, Math.min((i + 1) * 2, hexData.length()));
                byteValue = Integer.parseInt(byteStr, 16);
            } catch (NumberFormatException | StringIndexOutOfBoundsException e) {
                byteValue = 0;
            }

            // 解析每个位（左侧为低位，右侧为高位）
            for (int bit = 0; bit < 8; bit++) {
                canData[i][bit] = (byteValue >> bit) & 1;
            }
        }
    }

    // 更新信息面板
    private void updateInfoPanel() {
        Component[] components = ((JPanel) getContentPane().getComponent(0)).getComponents();
        if (components.length >= 4) {
            ((JLabel) components[0]).setText("CAN ID: " + (currentMessage.getCanId() != null ? currentMessage.getCanId() : ""));
            ((JLabel) components[1]).setText("DATA: " + (currentMessage.getData() != null ? currentMessage.getData() : ""));
        }
    }

    // 位单元格组件
    private class BitCell extends JPanel {
        private int byteIndex;
        private int bitIndex;

        public BitCell(int byteIndex, int bitIndex) {
            this.byteIndex = byteIndex;
            this.bitIndex = bitIndex;
            setPreferredSize(new Dimension(80, 50));
            setBorder(BorderFactory.createLineBorder(Color.LIGHT_GRAY));
            setLayout(new BorderLayout());

        }
        // 计算当前位的全局索引（关键：字节索引×8 + 位索引）
        private int getGlobalBitIndex() {
            return byteIndex * 8 + bitIndex;
        }

        @Override
        protected void paintComponent(Graphics g) {
            super.paintComponent(g);

            // 设置背景色
            Color bgColor = getBitBackgroundColor();
            setBackground(bgColor);


            // 绘制位值
            g.setColor(Color.BLACK);
            Font font = new Font("Arial", Font.BOLD, 16);
            g.setFont(font);

            String value = (currentMessage != null && byteIndex < canData.length && bitIndex < canData[byteIndex].length)
                    ? String.valueOf(canData[byteIndex][bitIndex]) : "0";

            FontMetrics fm = g.getFontMetrics();
            int x = (getWidth() - fm.stringWidth(value)) / 2;
            int y = (getHeight() - fm.getHeight()) / 2 + fm.getAscent();
            g.drawString(value, x, y);

            // 在信号起始位绘制信号名
            String signalName = getSignalNameAtThisBit();
            if (signalName != null) {
                g.setFont(new Font("Arial", Font.PLAIN, 9));
                fm = g.getFontMetrics();
                int nameX = (getWidth() - fm.stringWidth(signalName)) / 2;
                // 确保文字在边界内
                if (nameX < 0) nameX = 0;
                g.drawString(signalName, nameX, 12);
            }
        }

        // 获取当前位的背景色
        private Color getBitBackgroundColor() {
            int currentGlobalBit = getGlobalBitIndex();
            for (Signal signal : signals) {
                int signalEndBit = signal.startBit + signal.length - 1;
                // 判断当前位是否在信号的[startBit, endBit]范围内
                if (currentGlobalBit >= signal.startBit && currentGlobalBit <= signalEndBit) {
                    return signal.color;
                }
            }
            return Color.LIGHT_GRAY; // 无信号的位用灰色
        }

        // 检查是否是信号起始位并返回信号名
        private String getSignalNameAtThisBit() {
            int currentGlobalBit = getGlobalBitIndex();
            for (Signal signal : signals) {
                if (currentGlobalBit == signal.startBit) {
                    return signal.name;
                }
            }
            return null;
        }
    }

    // 启动方法
    public static void showCanData(SendMessage message) {
        SwingUtilities.invokeLater(() -> {
            CanFdBitViewer viewer = new CanFdBitViewer(message);
            viewer.setMessage(message);
            viewer.createUI();
            viewer.setVisible(true);
        });
    }

    // 测试主方法
    public static void main(String[] args) {
        // 创建测试消息
        SendMessage testMessage = new SendMessage();
        testMessage.setCanId("14C");
        testMessage.setData("C707FE0060690204002C000000000080FFFF00FFFF25802200000000000000FE");
        testMessage.setTime("1753340239312");
        testMessage.setDirection("TX");
        testMessage.setIndexCounter(12345L);
        testMessage.setProjectId(17L);

        // 显示CAN数据
        showCanData(testMessage);
    }
}
