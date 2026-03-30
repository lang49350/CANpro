package org.example.ui;

import org.apache.commons.lang3.StringUtils;
import org.example.mapper.CProjectMapper;
import org.example.pojo.data.ValueChangConfig;
import org.example.pojo.project.CProject;
import org.example.ui.project.OpenHistoryProjectDialog;
import org.example.ui.project.ProjectFormDialog;
import org.example.ui.project.ProjectManager;
import org.example.ui.project.StandorTablePanel;
import org.example.ui.project.chart.group.GroupMangeView;
import org.example.ui.project.chart.group.SignalGroupDialog;
import org.example.ui.project.error.ErrorInfoDisplay;
import org.example.ui.project.events.EventConfigView;
import org.example.ui.project.events.ValueChangEventView;
import org.example.utils.chart.CombaFrame;
import org.example.utils.db.DatabaseManager;

import javax.swing.*;
import javax.swing.event.InternalFrameAdapter;
import javax.swing.event.InternalFrameEvent;
import javax.swing.filechooser.FileNameExtensionFilter;
import java.awt.*;
import java.awt.event.InputEvent;
import java.awt.event.KeyEvent;
import java.beans.PropertyVetoException;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.util.Date;
import java.util.List;
import java.util.Objects;

public class MainFrame extends JFrame {

    private JDesktopPane desktopPane;
    private JTextField statusField;
    private int windowCount = 0;
    public MainFrame() {
        // 设置主窗口
        setTitle("CAN数据分析");
        setSize(getMaximumSize());
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        setLocationRelativeTo(null);

        JPanel contentPane = new JPanel(new BorderLayout());

        // 创建菜单栏
        createMenuBar();

        // 创建工具栏
        JToolBar toolBar = createToolBar();
        contentPane.add(toolBar, BorderLayout.NORTH);

        // 创建状态栏
        JPanel statusBar = createStatusBar();
        contentPane.add(statusBar, BorderLayout.SOUTH);

        // 创建桌面面板（MDI容器）
        desktopPane = new JDesktopPane();
        contentPane.add(desktopPane, BorderLayout.CENTER);
        // 设置内容面板
        setContentPane(contentPane);
    }

    public void setStatusBarText(String text){
        statusField.setText(text);
    }

    private void createMenuBar() {
        JMenuBar menuBar = new JMenuBar();

        // 文件菜单
        JMenu fileMenu = new JMenu("文件");
        fileMenu.setMnemonic(KeyEvent.VK_F);

        JMenuItem newMenuItem = new JMenuItem("新建", KeyEvent.VK_N);
        newMenuItem.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_N, InputEvent.CTRL_DOWN_MASK));
        newMenuItem.addActionListener(e -> createNewDocument());

        JMenuItem openMenuItem = new JMenuItem("OPEN", KeyEvent.VK_O);
        openMenuItem.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_O, InputEvent.CTRL_DOWN_MASK));
        openMenuItem.addActionListener(e -> openHistoryProjects());

        JMenuItem closeMenuItem = new JMenuItem("关闭", KeyEvent.VK_C);
        closeMenuItem.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_W, InputEvent.CTRL_DOWN_MASK));
        closeMenuItem.addActionListener(e -> closeActiveDocument());

        JMenuItem exitMenuItem = new JMenuItem("退出", KeyEvent.VK_X);
        exitMenuItem.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_Q, InputEvent.CTRL_DOWN_MASK));
        exitMenuItem.addActionListener(e -> System.exit(0));

        fileMenu.add(newMenuItem);
        fileMenu.add(openMenuItem);
        fileMenu.add(closeMenuItem);
        fileMenu.addSeparator();
        fileMenu.add(exitMenuItem);

        // 窗口菜单
        JMenu windowMenu = new JMenu("窗口");
        windowMenu.setMnemonic(KeyEvent.VK_W);

        JMenuItem cascadeMenuItem = new JMenuItem("层叠窗口", KeyEvent.VK_C);
        cascadeMenuItem.addActionListener(e -> cascadeWindows());

        JMenuItem tileMenuItem = new JMenuItem("平铺窗口", KeyEvent.VK_T);
        tileMenuItem.addActionListener(e -> tileWindows());

        windowMenu.add(cascadeMenuItem);
        windowMenu.add(tileMenuItem);

        // 工具
        JMenu toolMenu = new JMenu("工具");

        JMenuItem signalGroupMenuItem = new JMenuItem("信号组配置", KeyEvent.VK_G);
        signalGroupMenuItem.addActionListener(e ->{
            GroupMangeView signalGroupDialog = new GroupMangeView(false);
            signalGroupDialog.setVisible(true);
        });

        JMenuItem blfToJsonMenuItem = new JMenuItem("BLF文件转JSON");
        blfToJsonMenuItem.addActionListener(e ->{
            // 打开文件选择器选择blf文件
            JFileChooser fileChooser = new JFileChooser();
            fileChooser.setFileFilter(new FileNameExtensionFilter("BLF文件", "blf"));
            int returnValue = fileChooser.showOpenDialog(this);
            if (returnValue == JFileChooser.APPROVE_OPTION) {
                File selectedFile = fileChooser.getSelectedFile();
                // blf 文件转json文件
                ProcessBuilder processBuilder = new
                        ProcessBuilder("blf.exe",selectedFile.getAbsolutePath());
                try {
                    Process process = processBuilder.start();
                    // 读取标准输出（JSON数据）
                    StringBuilder output = new StringBuilder();
                    try (BufferedReader reader = new BufferedReader(
                            new InputStreamReader(process.getInputStream()))) {
                        String line;
                        while ((line = reader.readLine()) != null) {
                            output.append(line);
                        }
                    }
                    if(StringUtils.isNotBlank(output.toString()) && output.toString().contains("SUCCESS")){
                        JOptionPane.showMessageDialog(null, "转换成功 ：" + output.toString());
                    }
                } catch (IOException ex) {
                    throw new RuntimeException(ex);
                }
            }

        });

        // BLF 转google protobuf 文件
        JMenuItem blfToProtobufMenuItem = new JMenuItem("BLF文件转protobuf");
        blfToProtobufMenuItem.addActionListener(e ->{
            // 打开文件选择器选择blf文件
            JFileChooser fileChooser = new JFileChooser();
            fileChooser.setFileFilter(new FileNameExtensionFilter("BLF文件", "blf"));
            int returnValue = fileChooser.showOpenDialog(this);
            File blfFile = null;
            if (returnValue == JFileChooser.APPROVE_OPTION) {
                blfFile = fileChooser.getSelectedFile();
                JFileChooser fileChooser2 = new JFileChooser();
                fileChooser2.setFileFilter(new FileNameExtensionFilter("protobuf文件(.pb文件)", "pb"));
                int returnValue2 = fileChooser2.showOpenDialog(this);
                if (returnValue2 == JFileChooser.APPROVE_OPTION) {
                    File pbFile = fileChooser2.getSelectedFile();
                    // blf 文件转protobuf文件
                    ProcessBuilder processBuilder = new
                            ProcessBuilder("blf_to_protobuf_batch_2.exe",
                            blfFile.getAbsolutePath(),pbFile.getAbsolutePath());
                    try {
                        Process process = processBuilder.start();
                        ErrorInfoDisplay errorInfoDisplay = new ErrorInfoDisplay();
                        errorInfoDisplay.setTitle("读取信息");
                        errorInfoDisplay.setVisible(true);
                        // 关键：单独线程读取输入流，避免阻塞
                        new Thread(() -> readProcessOutput(process.getInputStream(), errorInfoDisplay)).start();
                    } catch (IOException ex) {
                        throw new RuntimeException(ex);
                    }
                }
            }
        });

        JMenuItem dbcMenuItem = new JMenuItem("导入DBC文件");
        dbcMenuItem.addActionListener(e ->{
            // 打开文件选择器选择blf文件
            JFileChooser fileChooser = new JFileChooser();
            fileChooser.setFileFilter(new FileNameExtensionFilter("DBC文件", "dbc"));
            int returnValue = fileChooser.showOpenDialog(this);
            if (returnValue == JFileChooser.APPROVE_OPTION) {
                File selectedFile = fileChooser.getSelectedFile();
                // 导入DBC
                ProcessBuilder processBuilder = new
                        ProcessBuilder("dbc_parser_to_db_v2.exe",selectedFile.getAbsolutePath(),"dbc.db");
                try {
                    Process process = processBuilder.start();
                    StringBuilder output = new StringBuilder();
                    try (BufferedReader reader = new BufferedReader(
                            new InputStreamReader(process.getInputStream()))) {
                        String line;
                        while ((line = reader.readLine()) != null) {
                            output.append(line);
                        }
                    }
                    if(StringUtils.isNotBlank(output.toString()) && output.toString().contains("DBC文件解析并存储成功")){
                        JOptionPane.showMessageDialog(null, "导入成功 ：" );
                    }else{
                        System.out.println(output.toString());
                        JOptionPane.showMessageDialog(null, "导入失败 ：" + output.toString());
                    }
                } catch (IOException ex) {
                    throw new RuntimeException(ex);
                }
            }

        });

        JMenuItem combaChartMenuItem = new JMenuItem("组合图表");
        combaChartMenuItem.addActionListener(e ->{
            CombaFrame combaFrame = new CombaFrame();
            combaFrame.setVisible(true);
        });

        JMenuItem eventConfigMenuItem = new JMenuItem("休眠事件配置");
        eventConfigMenuItem.addActionListener(e ->{
             EventConfigView eventConfigView = new EventConfigView(this);
            eventConfigView.setVisible(true);
        });

        JMenuItem valChangeEventConfigMenuItem = new JMenuItem("值变化事件配置");
        valChangeEventConfigMenuItem.addActionListener(e ->{
            ValueChangEventView eventConfigView = new ValueChangEventView(null);
            eventConfigView.setVisible(true);
        });


        toolMenu.add(signalGroupMenuItem);
        toolMenu.add(blfToJsonMenuItem);
        toolMenu.add(blfToProtobufMenuItem);
        toolMenu.add(dbcMenuItem);
        toolMenu.add(combaChartMenuItem);
        toolMenu.add(eventConfigMenuItem);
        toolMenu.add(valChangeEventConfigMenuItem);

        menuBar.add(fileMenu);
        menuBar.add(windowMenu);
        menuBar.add(toolMenu);
        setJMenuBar(menuBar);
    }

    /**
     * 读取进程输出并实时显示
     */
    private void readProcessOutput(InputStream inputStream, ErrorInfoDisplay display) {
        try (BufferedReader reader = new BufferedReader(
                new InputStreamReader(inputStream, StandardCharsets.UTF_8))) { // 指定编码，避免乱码
            String line;
            // 禁用缓冲或强制刷新（BufferedReader默认有8192字符缓冲）
            while ((line = reader.readLine()) != null) {
                System.out.println(line); // 控制台输出
                display.addErrorInfo(line); // 实时更新UI

                // 可选：如果仍然有延迟，可添加微小休眠让EDT有时间处理

            }
        } catch (Exception e) {
            e.printStackTrace();
            display.addErrorInfo("读取输出失败: " + e.getMessage());
        }
    }

    private void openHistoryProjects() {
        DatabaseManager databaseManager = new DatabaseManager();
        List<CProject> projects = databaseManager.execute(CProject.class, session->{
            CProjectMapper mapper = session.getMapper(CProjectMapper.class);
            return mapper.selectList(null);
        });
        OpenHistoryProjectDialog dialog = new OpenHistoryProjectDialog(this,projects);
        dialog.setVisible(true);
        if(dialog.isFileSelected()){
            CProject project = dialog.getSelectedFile();
            doCreateNewProject(project);
        }
    }

    private JToolBar createToolBar() {
        JToolBar toolBar = new JToolBar();
        toolBar.setFloatable(false);

        JButton newButton = new JButton(new ImageIcon(Objects.requireNonNull(getClass().getResource("/icons/create_new.png"))));
        newButton.setToolTipText("新建工程");
        newButton.addActionListener(e -> createNewDocument());

        toolBar.add(newButton);
        toolBar.addSeparator();

        return toolBar;
    }

    private JPanel createStatusBar() {
        JPanel statusPanel = new JPanel(new BorderLayout());
        statusPanel.setBorder(BorderFactory.createEtchedBorder());

        statusField = new JTextField("就绪");
        statusField.setEditable(false);
        statusField.setBorder(null);
        statusPanel.add(statusField, BorderLayout.WEST);

        return statusPanel;
    }

    private void createNewDocument() {
        ProjectFormDialog dialog = new ProjectFormDialog(this);
        dialog.setVisible(true);
        if(dialog.getIsSelected()){
            CProject project = dialog.getProject();
            doCreateNewProject(project);
        }
    }

    private void doCreateNewProject(CProject project) {
        windowCount++;
        ProjectManager.getInstance().addProject(project);
        JInternalFrame frame = new JInternalFrame(
                StringUtils.defaultIfEmpty(project.getName(),"新工程"),
                true, true, true, true);
        frame.setSize(800, 600);
        frame.setLocation(30 * (windowCount % 5), 30 * (windowCount % 5));

        StandorTablePanel standorTablePanel = new StandorTablePanel(project.getId());
        standorTablePanel.setParentFrame(this);
        frame.add(standorTablePanel);
        frame.addInternalFrameListener(new InternalFrameAdapter() {
            @Override
            public void internalFrameActivated(InternalFrameEvent e) {
                updateStatus("当前活动窗口: " + frame.getTitle());
            }

            @Override
            public void internalFrameClosed(InternalFrameEvent e) {
                updateStatus("窗口已关闭: " + frame.getTitle());
            }
            @Override
            public void internalFrameClosing(InternalFrameEvent e) {
                int result = JOptionPane.showConfirmDialog(
                        frame,
                        "确定要关闭工程吗？未保存的更改将丢失。",
                        "确认关闭",
                        JOptionPane.YES_NO_OPTION,
                        JOptionPane.QUESTION_MESSAGE
                );
                System.out.println("result:" + result);
                if (result == JOptionPane.YES_OPTION) {
                    // 用户确认关闭，继续默认关闭操作
                    frame.dispose();
                } else {
                    // 用户取消关闭，取消关闭操作
                    e.getInternalFrame().setDefaultCloseOperation(JInternalFrame.DO_NOTHING_ON_CLOSE);
                }
            }
        });

        desktopPane.add(frame);
        frame.setVisible(true);

        try {
            frame.setSelected(true);
        } catch (java.beans.PropertyVetoException ex) {
            ex.printStackTrace();
        }

        updateStatus("创建新工程: " + frame.getTitle());
    }


    private void closeActiveDocument() {
        JInternalFrame activeFrame = desktopPane.getSelectedFrame();
        if (activeFrame != null) {
            try {
                updateStatus("关闭工程: " + activeFrame.getTitle());
                activeFrame.setClosed(true);
            } catch (java.beans.PropertyVetoException ex) {
                ex.printStackTrace();
            }
        } else {
            updateStatus("没有活动窗口");
        }
    }

    private void cascadeWindows() {
        JInternalFrame[] frames = desktopPane.getAllFrames();
        int x = 0, y = 0;
        int offset = 30;

        for (JInternalFrame frame : frames) {
            frame.setLocation(x, y);
            x += offset;
            y += offset;
        }

        updateStatus("窗口已层叠排列");
    }

    private void tileWindows() {
        JInternalFrame[] frames = desktopPane.getAllFrames();
        if (frames.length == 0) return;

        Dimension desktopSize = desktopPane.getSize();
        int cols = (int) Math.ceil(Math.sqrt(frames.length));
        int rows = (int) Math.ceil((double) frames.length / cols);
        int width = desktopSize.width / cols;
        int height = desktopSize.height / rows;

        int currentRow = 0, currentCol = 0;
        for (JInternalFrame frame : frames) {
            frame.setSize(width - 10, height - 10);
            frame.setLocation(currentCol * width, currentRow * height);

            currentCol++;
            if (currentCol >= cols) {
                currentCol = 0;
                currentRow++;
            }
        }

        updateStatus("窗口已平铺排列");
    }

    private void updateStatus(String message) {
        statusField.setText(message + " - " + new Date().toString());
    }

}
