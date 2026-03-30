#include <QApplication>
#include <QFile>
#include <QDebug>
#include <QIcon>
#include "ui/MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // 设置应用程序信息
    app.setApplicationName("CANAnalyzerPro");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("CANTools");
    app.setOrganizationDomain("cantools.com");
    
    // 设置应用程序图标（任务栏图标）
    app.setWindowIcon(QIcon(":/logo.png"));
    
    // 加载样式表
    QFile styleFile(":/styles/app_styles.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        QString styleSheet = QString::fromUtf8(styleFile.readAll());
        
        // 替换图标路径占位符
        QString iconPath = ":/icons";
        styleSheet.replace("%IMAGE_DIR%", iconPath);
        
        app.setStyleSheet(styleSheet);
        styleFile.close();
        
        qDebug() << "✅ 样式表加载成功";
    } else {
        qWarning() << "⚠️ 无法加载样式表";
    }
    
    // 创建并显示主窗口
    MainWindow mainWindow;
    mainWindow.show();
    
    qDebug() << "启动成功";
    
    return app.exec();
}
