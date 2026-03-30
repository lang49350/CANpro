/*
 * Created by JFormDesigner on Mon Jul 14 13:55:29 CST 2025
 */

package org.example.ui.project.send;

import javax.swing.border.*;
import lombok.Data;
import lombok.EqualsAndHashCode;

import java.awt.*;
import javax.swing.*;

/**
 * @author jingq
 */
@EqualsAndHashCode(callSuper = true)
@Data
public class SignalPanel extends JPanel {
    public SignalPanel() {
        initComponents();
    }

    public void setShowComment(boolean showComment) {
        this.showComment = showComment;
    }

    public void showComment(){
        if (showComment) {
            signalPanel2.setVisible(true);
        } else {
            signalPanel2.setVisible(false);
        }
    }

    private void initComponents() {
        // JFormDesigner - Component initialization - DO NOT MODIFY  //GEN-BEGIN:initComponents  @formatter:off
        signalsGroup = new JPanel();
        signalPanel = new JPanel();
        signalName = new JLabel();
        startBit = new JLabel();
        signalLength = new JLabel();
        byteOrder = new JLabel();
        isSigned = new JLabel();
        scale = new JLabel();
        offset = new JLabel();
        maxVal = new JLabel();
        minVal = new JLabel();
        unit = new JLabel();
        label11 = new JLabel();
        currentVal = new JLabel();
        label12 = new JLabel();
        setVal = new JTextField();
        signalPanel2 = new JPanel();
        signalComment = new JLabel();
        commentTextField = new JTextField();
        label13 = new JLabel();
        userCommentTextField = new JTextField();
        hSpacer1 = new JPanel(null);

        //======== this ========
        setLayout(new BoxLayout(this, BoxLayout.X_AXIS));

        //======== signalsGroup ========
        {
            signalsGroup.setBorder(LineBorder.createBlackLineBorder());
            signalsGroup.setLayout(new BoxLayout(signalsGroup, BoxLayout.Y_AXIS));

            //======== signalPanel ========
            {
                signalPanel.setLayout(new GridLayout());

                //---- signalName ----
                signalName.setText("\u4fe1\u53f7\u540d\u79f0:");
                signalPanel.add(signalName);

                //---- startBit ----
                startBit.setText("\u8d77\u59cb\u4f4d:");
                signalPanel.add(startBit);

                //---- signalLength ----
                signalLength.setText("\u957f\u5ea6:");
                signalPanel.add(signalLength);

                //---- byteOrder ----
                byteOrder.setText("\u5b57\u8282\u5e8f\u5217:");
                signalPanel.add(byteOrder);

                //---- isSigned ----
                isSigned.setText("\u662f\u5426\u6709\u7b26\u53f7:");
                signalPanel.add(isSigned);

                //---- scale ----
                scale.setText("\u7f29\u653e:");
                signalPanel.add(scale);

                //---- offset ----
                offset.setText("\u504f\u79fb:");
                signalPanel.add(offset);

                //---- maxVal ----
                maxVal.setText("\u6700\u5927\u503c:");
                signalPanel.add(maxVal);

                //---- minVal ----
                minVal.setText("\u6700\u5c0f\u503c:");
                signalPanel.add(minVal);

                //---- unit ----
                unit.setText("\u5355\u4f4d:");
                signalPanel.add(unit);

                //---- label11 ----
                label11.setText("\u5f53\u524d\u503c");
                signalPanel.add(label11);

                //---- currentVal ----
                currentVal.setText("text");
                signalPanel.add(currentVal);

                //---- label12 ----
                label12.setText("\u8bbe\u5b9a\u503c:");
                signalPanel.add(label12);

                //---- setVal ----
                setVal.setColumns(8);
                signalPanel.add(setVal);
            }
            signalsGroup.add(signalPanel);

            //======== signalPanel2 ========
            {
                signalPanel2.setLayout(new GridLayout());

                //---- signalComment ----
                signalComment.setText("\u4fe1\u53f7\u89e3\u91ca:");
                signalPanel2.add(signalComment);

                //---- commentTextField ----
                commentTextField.setColumns(20);
                commentTextField.setEditable(false);
                signalPanel2.add(commentTextField);

                //---- label13 ----
                label13.setText("\u7528\u6237\u5907\u6ce8:");
                signalPanel2.add(label13);

                //---- userCommentTextField ----
                userCommentTextField.setColumns(20);
                userCommentTextField.setEditable(false);
                signalPanel2.add(userCommentTextField);
                signalPanel2.add(hSpacer1);
            }
            signalsGroup.add(signalPanel2);
        }
        add(signalsGroup);
        // JFormDesigner - End of component initialization  //GEN-END:initComponents  @formatter:on
    }

    // JFormDesigner - Variables declaration - DO NOT MODIFY  //GEN-BEGIN:variables  @formatter:off
    private JPanel signalsGroup;
    private JPanel signalPanel;
    private JLabel signalName;
    private JLabel startBit;
    private JLabel signalLength;
    private JLabel byteOrder;
    private JLabel isSigned;
    private JLabel scale;
    private JLabel offset;
    private JLabel maxVal;
    private JLabel minVal;
    private JLabel unit;
    private JLabel label11;
    private JLabel currentVal;
    private JLabel label12;
    private JTextField setVal;
    private JPanel signalPanel2;
    private JLabel signalComment;
    private JTextField commentTextField;
    private JLabel label13;
    private JTextField userCommentTextField;
    private JPanel hSpacer1;
    // JFormDesigner - End of variables declaration  //GEN-END:variables  @formatter:on

    private boolean showComment = true;
}
