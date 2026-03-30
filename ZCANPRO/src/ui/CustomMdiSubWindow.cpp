/**
 * @file CustomMdiSubWindow.cpp
 * @brief 自定义MDI子窗口实现
 * @author CANAnalyzerPro Team
 * @date 2024-02-08
 */

#include "CustomMdiSubWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMdiArea>
#include <QDebug>

CustomMdiSubWindow::CustomMdiSubWindow(QWidget *parent)
    : QMdiSubWindow(parent)
    , m_dragging(false)
    , m_dragThresholdExceeded(false)
    , m_resizing(false)
    , m_resizeEdge(None)
    , m_borderWidth(8)  // 从5增加到8，更容易触发调整大小
    , m_contentWidget(nullptr)
    , m_isMaximized(false)
    , m_isMinimized(false)
{
    // 设置无边框
    setWindowFlags(Qt::FramelessWindowHint);
    
    // 🔧 BUG修复：设置窗口在关闭时自动销毁，确保destroyed信号被触发
    setAttribute(Qt::WA_DeleteOnClose, true);
    
    // 设置样式 - 添加2px边框作为视觉提示
    setStyleSheet(
        "QMdiSubWindow { "
        "   border: 2px solid #607D8B; "  // 添加边框
        "   background-color: white; "
        "}"
    );
    
    // 启用鼠标跟踪，用于边框检测
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover, true);  // 启用hover事件
}

CustomMdiSubWindow::~CustomMdiSubWindow()
{
}

void CustomMdiSubWindow::setWidget(QWidget *widget)
{
    if (!widget) {
        return;
    }
    
    // 保存用户widget的引用
    m_contentWidget = widget;
    
    // 创建容器widget
    QWidget *container = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(container);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    
    // 添加自定义标题栏
    m_titleBar = createTitleBar();
    layout->addWidget(m_titleBar);
    
    // 添加用户的widget
    layout->addWidget(widget);
    
    // 设置容器为子窗口的widget
    QMdiSubWindow::setWidget(container);
}

QWidget* CustomMdiSubWindow::createTitleBar()
{
    QWidget *titleBar = new QWidget();
    titleBar->setMinimumHeight(40);
    titleBar->setMaximumHeight(40);
    titleBar->setStyleSheet("QWidget { background-color: #607D8B; }");
    
    // 确保标题栏可以接收鼠标事件
    titleBar->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    
    QHBoxLayout *layout = new QHBoxLayout(titleBar);
    layout->setSpacing(0);
    layout->setContentsMargins(10, 0, 0, 0);
    
    // 标题文字
    m_lblTitle = new QLabel("窗口");
    m_lblTitle->setStyleSheet("color: white; font-size: 12px; font-weight: bold;");
    layout->addWidget(m_lblTitle);
    
    layout->addStretch();
    
    // 按钮样式
    QString btnStyle = 
        "QPushButton { "
        "   background-color: transparent; "
        "   color: white; "
        "   border: none; "
        "   padding: 0px; "
        "   font-size: 14px; "
        "   min-width: 35px; "
        "   max-width: 35px; "
        "   min-height: 35px; "
        "   max-height: 35px; "
        "} "
        "QPushButton:hover { "
        "   background-color: rgba(255,255,255,0.2); "
        "}";
    
    QString closeBtnStyle = btnStyle + 
        "QPushButton:hover { "
        "   background-color: #E81123; "
        "}";
    
    // 最小化按钮
    m_btnMinimize = new QPushButton("—");
    m_btnMinimize->setStyleSheet(btnStyle);
    m_btnMinimize->setToolTip("最小化");
    connect(m_btnMinimize, &QPushButton::clicked, this, [this]() {
        qDebug() << "🖱️ 最小化按钮被点击:" << windowTitle();
        onMinimizeClicked();
    });
    layout->addWidget(m_btnMinimize);
    
    // 最大化按钮
    m_btnMaximize = new QPushButton("□");
    m_btnMaximize->setStyleSheet(btnStyle);
    m_btnMaximize->setToolTip("最大化");
    connect(m_btnMaximize, &QPushButton::clicked, this, [this]() {
        qDebug() << "🖱️ 最大化按钮被点击:" << windowTitle();
        onMaximizeClicked();
    });
    layout->addWidget(m_btnMaximize);
    
    // 关闭按钮
    m_btnClose = new QPushButton("✕");
    m_btnClose->setStyleSheet(closeBtnStyle);
    m_btnClose->setToolTip("关闭");
    connect(m_btnClose, &QPushButton::clicked, this, [this]() {
        qDebug() << "🖱️ 关闭按钮被点击:" << windowTitle();
        onCloseClicked();
    });
    layout->addWidget(m_btnClose);
    
    return titleBar;
}

void CustomMdiSubWindow::setWindowTitle(const QString &title)
{
    QMdiSubWindow::setWindowTitle(title);
    if (m_lblTitle) {
        m_lblTitle->setText(title);
    }
}

void CustomMdiSubWindow::onMinimizeClicked()
{
    // 获取MDI区域：QMdiSubWindow有mdiArea()方法
    QMdiArea *mdi = QMdiSubWindow::mdiArea();
    if (!mdi) {
        qWarning() << "⚠️ 无法获取MDI区域";
        return;
    }
    
    if (m_isMinimized) {
        // 恢复正常大小
        qDebug() << "🔼 恢复窗口:" << windowTitle() << "从" << geometry() << "到" << m_minimizedGeometry;
        
        // 重置大小限制
        setMinimumSize(200, 150);
        setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        
        // 显示内容widget
        if (m_contentWidget) {
            m_contentWidget->show();
        }
        
        setGeometry(m_minimizedGeometry);
        m_isMinimized = false;
        
        // 恢复按钮文字
        m_btnMinimize->setText("—");
        m_btnMaximize->setText("□");
        m_btnMaximize->setEnabled(true);
        
        raise();  // 提升到最前面
        activateWindow();
        
        // 确保其他最小化窗口仍然在最上层
        QList<QMdiSubWindow*> subWindows = mdi->subWindowList();
        for (QMdiSubWindow *subWin : subWindows) {
            CustomMdiSubWindow *customWin = qobject_cast<CustomMdiSubWindow*>(subWin);
            if (customWin && customWin->isMinimized()) {
                customWin->raise();  // 提升所有最小化窗口到最上层
            }
        }
    } else {
        // 保存当前几何位置
        m_minimizedGeometry = geometry();
        qDebug() << "🔽 最小化窗口:" << windowTitle() << "保存位置:" << m_minimizedGeometry;
        
        // 隐藏内容widget，只保留标题栏
        if (m_contentWidget) {
            m_contentWidget->hide();
        }
        
        // 最小化到左下角，只显示标题栏
        int miniWidth = 200;
        int miniHeight = 35;  // 只有标题栏高度
        
        // 计算位置：横向排列已最小化的窗口
        int x = 10;  // 起始位置
        int y = mdi->viewport()->height() - miniHeight - 10;
        
        // 检查其他已最小化的窗口，避免重叠
        QList<QMdiSubWindow*> subWindows = mdi->subWindowList();
        for (QMdiSubWindow *subWin : subWindows) {
            CustomMdiSubWindow *customWin = qobject_cast<CustomMdiSubWindow*>(subWin);
            if (customWin && customWin != this && customWin->isMinimized()) {
                // 如果有其他最小化窗口，计算新位置
                QRect otherGeom = customWin->geometry();
                if (otherGeom.y() > mdi->viewport()->height() - miniHeight - 20) {
                    // 这是一个最小化的窗口，计算下一个位置
                    int nextX = otherGeom.right() + 10;  // 右边10px间距
                    if (nextX > x) {
                        x = nextX;
                    }
                }
            }
        }
        
        qDebug() << "   设置最小化位置:" << x << y << miniWidth << "x" << miniHeight;
        
        // 设置固定大小（只显示标题栏）
        setMinimumSize(miniWidth, miniHeight);
        setMaximumSize(miniWidth, miniHeight);
        setGeometry(x, y, miniWidth, miniHeight);
        m_isMinimized = true;
        
        // 改变按钮文字，让用户知道这是恢复按钮
        m_btnMinimize->setText("🔼");  // 向上箭头表示恢复
        m_btnMaximize->setText("□");
        m_btnMaximize->setEnabled(false);  // 最小化状态下禁用最大化按钮
        
        // 确保最小化窗口在最上层，可以被点击
        raise();
        
        // 强制重绘，确保窗口可见
        update();
    }
}

void CustomMdiSubWindow::onMaximizeClicked()
{
    // 获取MDI区域：QMdiSubWindow有mdiArea()方法
    QMdiArea *mdi = QMdiSubWindow::mdiArea();
    if (!mdi) {
        qWarning() << "⚠️ 无法获取MDI区域";
        return;
    }
    
    if (m_isMaximized) {
        // 恢复正常大小
        qDebug() << "📐 恢复窗口:" << windowTitle() << "从" << geometry() << "到" << m_normalGeometry;
        setGeometry(m_normalGeometry);
        m_btnMaximize->setText("□");
        m_btnMaximize->setToolTip("最大化");
        m_isMaximized = false;
        
        // 确保其他最小化窗口仍然在最上层
        QList<QMdiSubWindow*> subWindows = mdi->subWindowList();
        for (QMdiSubWindow *subWin : subWindows) {
            CustomMdiSubWindow *customWin = qobject_cast<CustomMdiSubWindow*>(subWin);
            if (customWin && customWin->isMinimized()) {
                customWin->raise();  // 提升所有最小化窗口到最上层
            }
        }
    } else {
        // 如果是从最小化状态最大化，先恢复
        if (m_isMinimized) {
            m_isMinimized = false;
            m_btnMinimize->setText("—");
            m_btnMaximize->setEnabled(true);
        }
        
        // 保存当前几何位置
        m_normalGeometry = geometry();
        qDebug() << "📐 最大化窗口:" << windowTitle() << "保存位置:" << m_normalGeometry;
        
        // 获取MDI区域的大小
        QRect mdiRect = mdi->viewport()->rect();
        qDebug() << "   MDI区域大小:" << mdiRect;
        
        // 设置为填满MDI区域
        setGeometry(mdiRect);
        m_btnMaximize->setText("❐");
        m_btnMaximize->setToolTip("还原");
        m_isMaximized = true;
        raise();
        
        qDebug() << "   设置后窗口位置:" << geometry();
        
        // 确保所有最小化窗口在最大化窗口之上
        QList<QMdiSubWindow*> subWindows = mdi->subWindowList();
        for (QMdiSubWindow *subWin : subWindows) {
            CustomMdiSubWindow *customWin = qobject_cast<CustomMdiSubWindow*>(subWin);
            if (customWin && customWin != this && customWin->isMinimized()) {
                customWin->raise();  // 提升所有最小化窗口到最上层
                qDebug() << "   提升最小化窗口:" << customWin->windowTitle();
            }
        }
    }
}

void CustomMdiSubWindow::onCloseClicked()
{
    close();
    qDebug() << "❌ 窗口关闭:" << windowTitle();
}

// ==================== 边框检测和调整大小 ====================

CustomMdiSubWindow::ResizeEdge CustomMdiSubWindow::getResizeEdge(const QPoint &pos)
{
    int edge = None;
    
    if (pos.x() <= m_borderWidth) {
        edge |= Left;
    } else if (pos.x() >= width() - m_borderWidth) {
        edge |= Right;
    }
    
    if (pos.y() <= m_borderWidth) {
        edge |= Top;
    } else if (pos.y() >= height() - m_borderWidth) {
        edge |= Bottom;
    }
    
    return static_cast<ResizeEdge>(edge);
}

void CustomMdiSubWindow::updateCursor(ResizeEdge edge)
{
    switch (edge) {
        case TopLeft:
        case BottomRight:
            setCursor(Qt::SizeFDiagCursor);
            break;
        case TopRight:
        case BottomLeft:
            setCursor(Qt::SizeBDiagCursor);
            break;
        case Top:
        case Bottom:
            setCursor(Qt::SizeVerCursor);
            break;
        case Left:
        case Right:
            setCursor(Qt::SizeHorCursor);
            break;
        default:
            setCursor(Qt::ArrowCursor);
            break;
    }
}

// ==================== 窗口拖动和调整大小实现 ====================

void CustomMdiSubWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // 如果已最小化，点击任何位置都恢复
        if (m_isMinimized) {
            onMinimizeClicked();  // 恢复窗口
            event->accept();
            return;
        }
        
        // 检查是否在边框上（用于调整大小）
        m_resizeEdge = getResizeEdge(event->pos());
        if (m_resizeEdge != None && !m_isMaximized) {
            m_resizing = true;
            m_resizeStartGeometry = geometry();
            m_resizeStartPos = event->globalPosition().toPoint();
            event->accept();
            return;
        }
        
        // 检查是否点击在标题栏区域（用于拖动）
        if (event->pos().y() <= 35) {
            // 记录按下位置，但不立即开始拖动（等待超过阈值）
            m_dragStartPos = event->globalPosition().toPoint();
            m_dragPosition = event->pos();
            m_dragging = true;
            m_dragThresholdExceeded = false;
            event->accept();
            return;
        }
    }
    QMdiSubWindow::mousePressEvent(event);
}

void CustomMdiSubWindow::mouseMoveEvent(QMouseEvent *event)
{
    // 如果正在调整大小
    if (m_resizing && (event->buttons() & Qt::LeftButton)) {
        QPoint delta = event->globalPosition().toPoint() - m_resizeStartPos;
        QRect newGeometry = m_resizeStartGeometry;
        
        // 根据边缘调整几何位置
        if (m_resizeEdge & Left) {
            newGeometry.setLeft(m_resizeStartGeometry.left() + delta.x());
        }
        if (m_resizeEdge & Right) {
            newGeometry.setRight(m_resizeStartGeometry.right() + delta.x());
        }
        if (m_resizeEdge & Top) {
            newGeometry.setTop(m_resizeStartGeometry.top() + delta.y());
        }
        if (m_resizeEdge & Bottom) {
            newGeometry.setBottom(m_resizeStartGeometry.bottom() + delta.y());
        }
        
        // 限制最小尺寸
        if (newGeometry.width() < 200) {
            if (m_resizeEdge & Left) {
                newGeometry.setLeft(m_resizeStartGeometry.right() - 200);
            } else {
                newGeometry.setRight(m_resizeStartGeometry.left() + 200);
            }
        }
        if (newGeometry.height() < 150) {
            if (m_resizeEdge & Top) {
                newGeometry.setTop(m_resizeStartGeometry.bottom() - 150);
            } else {
                newGeometry.setBottom(m_resizeStartGeometry.top() + 150);
            }
        }
        
        setGeometry(newGeometry);
        event->accept();
        return;
    }
    
    // 如果正在拖动标题栏
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        // 检查是否超过拖动阈值（10px）
        if (!m_dragThresholdExceeded) {
            QPoint currentPos = event->globalPosition().toPoint();
            int distance = (currentPos - m_dragStartPos).manhattanLength();
            if (distance < 10) {
                // 还没超过阈值，不移动窗口
                event->accept();
                return;
            }
            // 超过阈值，开始真正的拖动
            m_dragThresholdExceeded = true;
        }
        
        // 如果已最大化，拖动时先恢复正常大小
        if (m_isMaximized) {
            m_isMaximized = false;
            m_btnMaximize->setText("□");
            m_btnMaximize->setToolTip("最大化");
            
            // 计算恢复后的窗口位置，保持鼠标在标题栏的相对位置
            QSize normalSize = m_normalGeometry.size();
            QPoint globalMousePos = event->globalPosition().toPoint();
            
            // 计算鼠标在标题栏的相对位置（百分比）
            qreal relativeX = static_cast<qreal>(m_dragPosition.x()) / width();
            
            // 计算新窗口的左上角位置
            int newX = globalMousePos.x() - static_cast<int>(normalSize.width() * relativeX);
            int newY = globalMousePos.y() - m_dragPosition.y();
            
            // 获取MDI区域，确保窗口不会跑到屏幕外
            QMdiArea *mdi = QMdiSubWindow::mdiArea();
            if (mdi) {
                QRect mdiRect = mdi->viewport()->rect();
                // 限制X坐标
                if (newX < 0) newX = 0;
                if (newX + normalSize.width() > mdiRect.width()) {
                    newX = mdiRect.width() - normalSize.width();
                }
                // 限制Y坐标
                if (newY < 0) newY = 0;
                if (newY + normalSize.height() > mdiRect.height()) {
                    newY = mdiRect.height() - normalSize.height();
                }
            }
            
            setGeometry(QRect(QPoint(newX, newY), normalSize));
            
            // 添加视觉反馈：改变标题栏颜色
            m_titleBar->setStyleSheet("QWidget { background-color: #78909C; }");  // 稍亮的蓝灰色
        } else {
            // 移动窗口
            move(mapToParent(event->pos() - m_dragPosition));
            
            // 添加视觉反馈：改变标题栏颜色
            m_titleBar->setStyleSheet("QWidget { background-color: #78909C; }");  // 稍亮的蓝灰色
        }
        event->accept();
        return;
    }
    
    // 更新鼠标光标（显示可调整大小的提示）
    if (!m_resizing && !m_dragging && !m_isMaximized) {
        ResizeEdge edge = getResizeEdge(event->pos());
        updateCursor(edge);
    }
    
    QMdiSubWindow::mouseMoveEvent(event);
}

void CustomMdiSubWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // 恢复标题栏颜色
        if (m_dragging && m_dragThresholdExceeded) {
            m_titleBar->setStyleSheet("QWidget { background-color: #607D8B; }");
        }
        
        m_dragging = false;
        m_dragThresholdExceeded = false;
        m_resizing = false;
        m_resizeEdge = None;
        event->accept();
        return;
    }
    QMdiSubWindow::mouseReleaseEvent(event);
}

void CustomMdiSubWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // 检查是否双击在标题栏区域
        if (event->pos().y() <= 35) {
            qDebug() << "🖱️ 双击标题栏:" << windowTitle();
            onMaximizeClicked();  // 切换最大化/还原
            event->accept();
            return;
        }
    }
    QMdiSubWindow::mouseDoubleClickEvent(event);
}
