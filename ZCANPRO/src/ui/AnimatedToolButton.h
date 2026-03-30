#ifndef ANIMATEDTOOLBUTTON_H
#define ANIMATEDTOOLBUTTON_H

#include <QToolButton>
#include <QGraphicsDropShadowEffect>
#include <QEvent>
#include <QRect>

class AnimatedToolButton : public QToolButton
{
    Q_OBJECT

public:
    explicit AnimatedToolButton(QWidget *parent = nullptr);
    ~AnimatedToolButton();

protected:
    bool event(QEvent *e) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void startHoverAnimation();
    void startLeaveAnimation();

    QGraphicsDropShadowEffect *m_shadow;
    
    QRect m_originalGeometry;
    bool m_isHovered;
    
    // 目标值
    int m_targetYOffset;
    qreal m_targetScale;
    
    // 当前值
    int m_currentYOffset;
    qreal m_currentScale;
};

#endif // ANIMATEDTOOLBUTTON_H
