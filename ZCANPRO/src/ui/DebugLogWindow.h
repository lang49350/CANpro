#ifndef DEBUGLOGWINDOW_H
#define DEBUGLOGWINDOW_H

#include <QWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QtGlobal>
#include <QShowEvent>
#include <QHideEvent>

class DebugLogWindow : public QWidget
{
    Q_OBJECT

public:
    explicit DebugLogWindow(QWidget *parent = nullptr);
    ~DebugLogWindow();

    static DebugLogWindow* instance();
    static void log(const QString &message);

private slots:
    void onClearClicked();
    void onSaveClicked();
    void onAutoScrollChanged(int state);
    void onEnabledChanged(bool enabled);
    void onLevelChanged(int index);

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private:
    void setupUi();
    void appendLog(const QString &message);
    void installMessageHandler();
    void uninstallMessageHandler();
    static void qtMessageRouter(QtMsgType type, const QMessageLogContext &context, const QString &msg);

    QPlainTextEdit *m_logView;
    QCheckBox *m_chkEnabled;
    QComboBox *m_comboLevel;
    QPushButton *m_btnClear;
    QPushButton *m_btnSave;
    QCheckBox *m_chkAutoScroll;

    bool m_autoScroll;

    static DebugLogWindow *s_instance;
    static QtMessageHandler s_previousHandler;
    static bool s_handlerInstalled;
};

#endif // DEBUGLOGWINDOW_H
