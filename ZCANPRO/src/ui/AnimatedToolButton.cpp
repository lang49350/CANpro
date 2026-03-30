#include "AnimatedToolButton.h"
#include <QTimer>

AnimatedToolButton::AnimatedToolButton(QWidget *parent)
    : QToolButton(parent)
    , m_isHovered(false)
    , m_targetYOffset(0)
    , m_targetScale(1.0)
    , m_currentYOffset(0)
    , m_currentScale(1.0)
{
    // 创建阴影效果 - 使用深红色阴影与背景呼应
    m_shadow = new QGraphicsDropShadowEffect(this);
    m_shadow->setBlurRadius(8);
    m_shadow->setColor(QColor(139, 0, 0, 100));  // 深红色，透明度100
    m_shadow->setOffset(0, 2);
    setGraphicsEffect(m_shadow);

    // 启用鼠标追踪
    setAttribute(Qt::WA_Hover, true);
}

AnimatedToolButton::~AnimatedToolButton()
{
}

bool AnimatedToolButton::event(QEvent *e)
{
    if (e->type() == QEvent::HoverEnter) {
        startHoverAnimation();
    } else if (e->type() == QEvent::HoverLeave) {
        startLeaveAnimation();
    }
    return QToolButton::event(e);
}

void AnimatedToolButton::resizeEvent(QResizeEvent *event)
{
    QToolButton::resizeEvent(event);
    if (!m_isHovered) {
        m_originalGeometry = geometry();
    }
}

void AnimatedToolButton::startHoverAnimation()
{
    m_isHovered = true;
    
    // 保存原始几何
    if (m_currentYOffset == 0 && m_currentScale == 1.0) {
        m_originalGeometry = geometry();
    }
    
    // 直接应用目标值，不使用动画
    m_targetYOffset = -2;
    m_targetScale = 1.02;
    m_currentYOffset = m_targetYOffset;
    m_currentScale = m_targetScale;
    
    // 立即应用几何变换
    int newWidth = static_cast<int>(m_originalGeometry.width() * m_currentScale);
    int newHeight = static_cast<int>(m_originalGeometry.height() * m_currentScale);
    int offsetX = (newWidth - m_originalGeometry.width()) / 2;
    int offsetY = (newHeight - m_originalGeometry.height()) / 2;
    
    setGeometry(
        m_originalGeometry.x() - offsetX,
        m_originalGeometry.y() - offsetY + m_currentYOffset,
        newWidth,
        newHeight
    );
    
    // 增强阴影（保持平滑过渡）- 更深的红色
    m_shadow->setBlurRadius(12);
    m_shadow->setColor(QColor(100, 0, 0, 140));  // 更深的红色，透明度140
    m_shadow->setOffset(0, 4);
}

void AnimatedToolButton::startLeaveAnimation()
{
    m_isHovered = false;
    
    // 直接恢复原始状态，不使用动画
    m_targetYOffset = 0;
    m_targetScale = 1.0;
    m_currentYOffset = 0;
    m_currentScale = 1.0;
    
    // 立即恢复原始几何
    setGeometry(m_originalGeometry);
    
    // 恢复原始阴影（保持平滑过渡）
    m_shadow->setBlurRadius(8);
    m_shadow->setColor(QColor(139, 0, 0, 100));  // 深红色
    m_shadow->setOffset(0, 2);
}
