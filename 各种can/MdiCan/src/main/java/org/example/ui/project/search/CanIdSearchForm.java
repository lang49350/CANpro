/*
 * Created by JFormDesigner on Tue Sep 02 12:17:42 CST 2025
 */

package org.example.ui.project.search;

import org.example.pojo.data.CanMessage;
import org.example.ui.project.StandorTablePanel;
import org.example.utils.table.EnhancedTable;

import java.awt.*;
import java.awt.event.*;
import java.util.ArrayList;
import java.util.List;
import javax.swing.*;
import javax.swing.border.*;

/**
 * @author jingq
 */
public class CanIdSearchForm extends JDialog {
    public interface ReturnValueListener {
        void onReturnValue(String value);
    }

    private ReturnValueListener listener;
    private List<CanMessage> canMessageList;
    private ExtendedEnhancedTable<CanMessage> enhancedTable;
    public CanIdSearchForm(Window owner, List<CanMessage> canMessageList,ReturnValueListener listener) {
        super(owner);
        this.listener = listener;
        this.canMessageList = canMessageList;
        initComponents();
        enhancedTable = new ExtendedEnhancedTable<>(CanMessage.class, new ExtendedEnhancedTable.RowSelectionListener() {
            @Override
            public void onRowSelected(int rowIndex, Object rowData) {
                onSelectedRow(rowIndex,rowData);
            }
        });
        scrollPane1.setViewportView(enhancedTable);
        enhancedTable.appendBatch(canMessageList);

    }

    public  void onSelectedRow(int rowIndex,Object rowData) {
        if (rowData == null) {
            return;
        }
        System.out.println("row index  is :"+ rowIndex);
        CanMessage canMessage = (CanMessage) rowData;
        listener.onReturnValue(String.valueOf(rowIndex));
    }

    private void onSearch(ActionEvent e) {
        // TODO add your code here
        enhancedTable.setColumnFilter("canId", textField1.getText());
    }

    private void onInput(ActionEvent e) {
        // TODO add your code here
        this.onSearch(e);
    }

    private void initComponents() {
        // JFormDesigner - Component initialization - DO NOT MODIFY  //GEN-BEGIN:initComponents  @formatter:off
        dialogPane = new JPanel();
        contentPanel = new JPanel();
        panel1 = new JPanel();
        label1 = new JLabel();
        textField1 = new JTextField();
        scrollPane1 = new JScrollPane();
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
                contentPanel.setLayout(new BorderLayout(6, 6));

                //======== panel1 ========
                {
                    panel1.setLayout(new GridLayout());

                    //---- label1 ----
                    label1.setText("CAN \u6d88\u606f ID(\u65e00x\u524d\u7f00)");
                    panel1.add(label1);

                    //---- textField1 ----
                    textField1.addActionListener(e -> onInput(e));
                    panel1.add(textField1);
                }
                contentPanel.add(panel1, BorderLayout.NORTH);
                contentPanel.add(scrollPane1, BorderLayout.CENTER);
            }
            dialogPane.add(contentPanel, BorderLayout.CENTER);

            //======== buttonBar ========
            {
                buttonBar.setBorder(new EmptyBorder(12, 0, 0, 0));
                buttonBar.setLayout(new GridBagLayout());
                ((GridBagLayout)buttonBar.getLayout()).columnWidths = new int[] {0, 85, 80};
                ((GridBagLayout)buttonBar.getLayout()).columnWeights = new double[] {1.0, 0.0, 0.0};

                //---- okButton ----
                okButton.setText("search");
                okButton.addActionListener(e -> onSearch(e));
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
        setSize(800, 300);
        setLocationRelativeTo(getOwner());
        // JFormDesigner - End of component initialization  //GEN-END:initComponents  @formatter:on
    }

    // JFormDesigner - Variables declaration - DO NOT MODIFY  //GEN-BEGIN:variables  @formatter:off
    private JPanel dialogPane;
    private JPanel contentPanel;
    private JPanel panel1;
    private JLabel label1;
    private JTextField textField1;
    private JScrollPane scrollPane1;
    private JPanel buttonBar;
    private JButton okButton;
    private JButton cancelButton;
    // JFormDesigner - End of variables declaration  //GEN-END:variables  @formatter:on

    public static void main(String[] args) {
        CanIdSearchForm canIdSearchForm = new CanIdSearchForm(null,null,null);
        canIdSearchForm.setVisible(true);
    }
}
