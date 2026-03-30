/*
 * Created by JFormDesigner on Mon Sep 22 15:59:48 CST 2025
 */

package org.example.ui.project.events;

import org.example.pojo.data.CanMessage;
import org.example.utils.file.FileChooserUtils;
import org.example.utils.file.TxtFileReader;

import java.awt.*;
import java.awt.event.*;
import java.io.File;
import java.util.ArrayList;
import java.util.List;
import javax.swing.*;
import javax.swing.border.*;

/**
 * @author jingq
 */
public class TxtFileParseView extends JDialog {
    public TxtFileParseView(Window owner) {
        super(owner);
        initComponents();
    }

    private void loadTxt(ActionEvent e) {
        // TODO add your code here
        File file = FileChooserUtils.chooseTxtFile();
        String result = "";
        java.util.List<String> msgStrList = new ArrayList<>();
        if(file != null){
            java.util.List<String> strings = TxtFileReader.readFileToList(file);
            List<CanMessage> canMessageList = CanMessage.toCanMessageList(strings);
            for (CanMessage message : canMessageList){
                String canId = message.getCanId();
                String data = message.getData();
                msgStrList.add(canId + "_" + data);
            }
            textArea1.setText(String.join(",", msgStrList));
        }
    }

    private void initComponents() {
        // JFormDesigner - Component initialization - DO NOT MODIFY  //GEN-BEGIN:initComponents  @formatter:off
        dialogPane = new JPanel();
        contentPanel = new JPanel();
        panel1 = new JPanel();
        textArea1 = new JTextArea();
        button1 = new JButton();
        buttonBar = new JPanel();
        okButton = new JButton();
        cancelButton = new JButton();

        //======== this ========
        Container contentPane = getContentPane();
        contentPane.setLayout(new BorderLayout());

        //======== dialogPane ========
        {
            dialogPane.setBorder(new EmptyBorder(12, 12, 12, 12));
            dialogPane.setLayout(new BorderLayout());

            //======== contentPanel ========
            {
                contentPanel.setLayout(new BoxLayout(contentPanel, BoxLayout.X_AXIS));

                //======== panel1 ========
                {
                    panel1.setLayout(new BoxLayout(panel1, BoxLayout.X_AXIS));
                    panel1.add(textArea1);

                    //---- button1 ----
                    button1.setText("\u9009\u62e9\u6587\u4ef6");
                    button1.addActionListener(e -> loadTxt(e));
                    panel1.add(button1);
                }
                contentPanel.add(panel1);
            }
            dialogPane.add(contentPanel, BorderLayout.CENTER);

            //======== buttonBar ========
            {
                buttonBar.setBorder(new EmptyBorder(12, 0, 0, 0));
                buttonBar.setLayout(new GridBagLayout());
                ((GridBagLayout)buttonBar.getLayout()).columnWidths = new int[] {0, 85, 80};
                ((GridBagLayout)buttonBar.getLayout()).columnWeights = new double[] {1.0, 0.0, 0.0};

                //---- okButton ----
                okButton.setText("OK");
                buttonBar.add(okButton, new GridBagConstraints(1, 0, 1, 1, 0.0, 0.0,
                    GridBagConstraints.CENTER, GridBagConstraints.BOTH,
                    new Insets(0, 0, 0, 5), 0, 0));

                //---- cancelButton ----
                cancelButton.setText("Cancel");
                buttonBar.add(cancelButton, new GridBagConstraints(2, 0, 1, 1, 0.0, 0.0,
                    GridBagConstraints.CENTER, GridBagConstraints.BOTH,
                    new Insets(0, 0, 0, 0), 0, 0));
            }
            dialogPane.add(buttonBar, BorderLayout.SOUTH);
        }
        contentPane.add(dialogPane, BorderLayout.CENTER);
        pack();
        setLocationRelativeTo(getOwner());
        // JFormDesigner - End of component initialization  //GEN-END:initComponents  @formatter:on
    }

    // JFormDesigner - Variables declaration - DO NOT MODIFY  //GEN-BEGIN:variables  @formatter:off
    private JPanel dialogPane;
    private JPanel contentPanel;
    private JPanel panel1;
    private JTextArea textArea1;
    private JButton button1;
    private JPanel buttonBar;
    private JButton okButton;
    private JButton cancelButton;
    // JFormDesigner - End of variables declaration  //GEN-END:variables  @formatter:on
}
