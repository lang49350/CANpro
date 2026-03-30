/*
 * Created by JFormDesigner on Tue Jul 15 14:16:20 CST 2025
 */

package org.example.ui.project.send;

import java.awt.event.*;
import lombok.Data;
import lombok.EqualsAndHashCode;
import org.example.pojo.data.SendMessage;
import org.example.utils.table.EnhancedTable;

import java.awt.*;
import java.util.Arrays;
import javax.swing.*;
import javax.swing.border.*;

/**
 * @author jingq
 */
@EqualsAndHashCode(callSuper = true)
@Data
public class KeepListView extends JDialog {
    private EnhancedTable<SendMessage> enhancedTable;
    public KeepListView(Window owner) {
        super(owner);
        initComponents();
        init();
        setTitle("保留列表");
        setSize(new Dimension(600, 600));
    }

    private void init() {
        enhancedTable = new EnhancedTable<>(SendMessage.class);
        JScrollPane scrollPane = new JScrollPane(enhancedTable);
        enhancedTable.setUnShowColumns(Arrays.asList("data","time","indexCounter"));
        enhancedTable.initColumns();
        contentPanel.add(scrollPane, BorderLayout.CENTER);
    }

    private void ok(ActionEvent e) {
        // TODO add your code here
        dispose();
    }

    private void cancel(ActionEvent e) {
        // TODO add your code here
        dispose();
    }

    private void delete(ActionEvent e) {
        // TODO add your code here
        int selectedRow = enhancedTable.getSelectedRow();
        enhancedTable.removeDataAt(selectedRow);
    }

    private void initComponents() {
        // JFormDesigner - Component initialization - DO NOT MODIFY  //GEN-BEGIN:initComponents  @formatter:off
        dialogPane = new JPanel();
        contentPanel = new JPanel();
        buttonBar = new JPanel();
        button1 = new JButton();
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
                contentPanel.setLayout(new BorderLayout());
            }
            dialogPane.add(contentPanel, BorderLayout.PAGE_START);

            //======== buttonBar ========
            {
                buttonBar.setBorder(new EmptyBorder(12, 0, 0, 0));
                buttonBar.setLayout(new GridBagLayout());
                ((GridBagLayout)buttonBar.getLayout()).columnWidths = new int[] {0, 85, 80};
                ((GridBagLayout)buttonBar.getLayout()).columnWeights = new double[] {1.0, 0.0, 0.0};

                //---- button1 ----
                button1.setText("\u5220\u9664\u6240\u9009");
                button1.addActionListener(e -> delete(e));
                buttonBar.add(button1, new GridBagConstraints(0, 0, 1, 1, 0.0, 0.0,
                    GridBagConstraints.CENTER, GridBagConstraints.BOTH,
                    new Insets(0, 0, 0, 5), 0, 0));

                //---- okButton ----
                okButton.setText("OK");
                okButton.addActionListener(e -> ok(e));
                buttonBar.add(okButton, new GridBagConstraints(1, 0, 1, 1, 0.0, 0.0,
                    GridBagConstraints.CENTER, GridBagConstraints.BOTH,
                    new Insets(0, 0, 0, 5), 0, 0));

                //---- cancelButton ----
                cancelButton.setText("Cancel");
                cancelButton.addActionListener(e -> cancel(e));
                buttonBar.add(cancelButton, new GridBagConstraints(2, 0, 1, 1, 0.0, 0.0,
                    GridBagConstraints.CENTER, GridBagConstraints.BOTH,
                    new Insets(0, 0, 0, 0), 0, 0));
            }
            dialogPane.add(buttonBar, BorderLayout.PAGE_END);
        }
        contentPane.add(dialogPane, BorderLayout.CENTER);
        pack();
        setLocationRelativeTo(getOwner());
        // JFormDesigner - End of component initialization  //GEN-END:initComponents  @formatter:on
    }

    // JFormDesigner - Variables declaration - DO NOT MODIFY  //GEN-BEGIN:variables  @formatter:off
    private JPanel dialogPane;
    private JPanel contentPanel;
    private JPanel buttonBar;
    private JButton button1;
    private JButton okButton;
    private JButton cancelButton;
    // JFormDesigner - End of variables declaration  //GEN-END:variables  @formatter:on

    public static void main(String[] args) {
        KeepListView view = new KeepListView(null);
        view.setVisible(true);
    }
}
