/*
 * Created by JFormDesigner on Fri Sep 19 14:08:47 CST 2025
 */

package org.example.ui.project.chart.freeChart;

import java.awt.event.*;
import org.example.pojo.project.CProject;
import org.example.ui.project.ProjectManager;

import java.awt.*;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import javax.swing.*;
import javax.swing.border.*;

/**
 * @author jingq
 */
public class ProjectSelecter extends JDialog {
    public ProjectSelecter(Window owner) {
        super(owner);
        initComponents();
        init();
    }

    private void init() {
        this.setModal(true);
        Set<Map<Long, CProject>> projects = ProjectManager.getInstance().getProjects();
        for (Map<Long, CProject> projectMap : projects) {
            for (CProject project : projectMap.values()) {
                projectComb.addItem(project.getId() + "#" +project.getName());
            }
        }
    }

    public boolean getSelectedItem() {
        return projectComb.getSelectedItem() != null;
    }
    public Long getSelectedProjectId() {
        String selectedItem = Objects.requireNonNull(projectComb.getSelectedItem()).toString();
        String[] split = selectedItem.split("#");
        return Long.parseLong(split[0]);
    }

    private void ok(ActionEvent e) {
        // TODO add your code here
        dispose();
    }

    private void cancel(ActionEvent e) {
        // TODO add your code here
        projectComb.setSelectedItem(null);
        dispose();
    }

    private void onCancel(ActionEvent e) {
        // TODO add your code here
    }

    private void initComponents() {
        // JFormDesigner - Component initialization - DO NOT MODIFY  //GEN-BEGIN:initComponents  @formatter:off
        dialogPane = new JPanel();
        contentPanel = new JPanel();
        panel1 = new JPanel();
        projectComb = new JComboBox();
        buttonBar = new JPanel();
        okButton = new JButton();
        cancelButton = new JButton();

        //======== this ========
        setTitle("\u9009\u62e9\u6570\u636e\u6765\u6e90");
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
                }
                contentPanel.add(panel1);
                contentPanel.add(projectComb);
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
                okButton.addActionListener(e -> {
			ok(e);
			ok(e);
		});
                buttonBar.add(okButton, new GridBagConstraints(1, 0, 1, 1, 0.0, 0.0,
                    GridBagConstraints.CENTER, GridBagConstraints.BOTH,
                    new Insets(0, 0, 0, 5), 0, 0));

                //---- cancelButton ----
                cancelButton.setText("Cancel");
                cancelButton.addActionListener(e -> {
			cancel(e);
			onCancel(e);
			cancel(e);
		});
                buttonBar.add(cancelButton, new GridBagConstraints(2, 0, 1, 1, 0.0, 0.0,
                    GridBagConstraints.CENTER, GridBagConstraints.BOTH,
                    new Insets(0, 0, 0, 0), 0, 0));
            }
            dialogPane.add(buttonBar, BorderLayout.SOUTH);
        }
        contentPane.add(dialogPane, BorderLayout.CENTER);
        setSize(595, 385);
        setLocationRelativeTo(getOwner());
        // JFormDesigner - End of component initialization  //GEN-END:initComponents  @formatter:on
    }

    // JFormDesigner - Variables declaration - DO NOT MODIFY  //GEN-BEGIN:variables  @formatter:off
    private JPanel dialogPane;
    private JPanel contentPanel;
    private JPanel panel1;
    private JComboBox projectComb;
    private JPanel buttonBar;
    private JButton okButton;
    private JButton cancelButton;
    // JFormDesigner - End of variables declaration  //GEN-END:variables  @formatter:on
}
