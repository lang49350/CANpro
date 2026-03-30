/**
 * @file HighlightDelegate.cpp
 * @brief 高亮显示委托实现
 */

#include "HighlightDelegate.h"
#include <QDebug>

HighlightDelegate::HighlightDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void HighlightDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                               const QModelIndex &index) const
{
    // 检查该列是否有高亮关键词
    if (!m_highlightKeywords.contains(index.column())) {
        // 没有高亮，使用默认绘制
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }
    
    QString keyword = m_highlightKeywords[index.column()];
    if (keyword.isEmpty()) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }
    
    // 获取显示文本
    QString text = index.data(Qt::DisplayRole).toString();
    if (text.isEmpty()) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }
    
    // 获取文本颜色
    QColor textColor = index.data(Qt::ForegroundRole).value<QColor>();
    if (!textColor.isValid()) {
        textColor = option.palette.color(QPalette::Text);
    }
    
    // 绘制背景和选中状态
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    
    // 绘制背景
    painter->save();
    if (opt.state & QStyle::State_Selected) {
        painter->fillRect(opt.rect, opt.palette.highlight());
    } else {
        painter->fillRect(opt.rect, opt.palette.base());
    }
    painter->restore();
    
    // 生成高亮HTML
    QString html = generateHighlightHtml(text, keyword, textColor);
    
    // 使用QTextDocument渲染HTML
    QTextDocument doc;
    doc.setHtml(html);
    doc.setDefaultFont(opt.font);
    doc.setTextWidth(opt.rect.width());
    
    // 设置对齐方式
    Qt::Alignment alignment = index.data(Qt::TextAlignmentRole).value<Qt::Alignment>();
    QTextOption textOption;
    if (alignment & Qt::AlignRight) {
        textOption.setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    } else if (alignment & Qt::AlignHCenter) {
        textOption.setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    } else {
        textOption.setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    }
    doc.setDefaultTextOption(textOption);
    
    // 绘制文本
    painter->save();
    painter->translate(opt.rect.topLeft());
    
    // 垂直居中
    qreal yOffset = (opt.rect.height() - doc.size().height()) / 2.0;
    painter->translate(0, yOffset);
    
    QAbstractTextDocumentLayout::PaintContext ctx;
    if (opt.state & QStyle::State_Selected) {
        ctx.palette.setColor(QPalette::Text, opt.palette.color(QPalette::HighlightedText));
    } else {
        ctx.palette.setColor(QPalette::Text, textColor);
    }
    
    doc.documentLayout()->draw(painter, ctx);
    painter->restore();
}

QSize HighlightDelegate::sizeHint(const QStyleOptionViewItem &option,
                                   const QModelIndex &index) const
{
    return QStyledItemDelegate::sizeHint(option, index);
}

void HighlightDelegate::setHighlightKeyword(int column, const QString &keyword)
{
    m_highlightKeywords[column] = keyword;
}

void HighlightDelegate::clearHighlightKeywords()
{
    m_highlightKeywords.clear();
}

QString HighlightDelegate::generateHighlightHtml(const QString &text, const QString &keyword, const QColor &textColor) const
{
    if (keyword.isEmpty() || text.isEmpty()) {
        return QString("<span style='color:%1'>%2</span>")
            .arg(textColor.name())
            .arg(text.toHtmlEscaped());
    }
    
    QString result = text;
    QString colorName = textColor.name();
    
    // 查找所有匹配位置（不区分大小写）
    int pos = 0;
    QList<QPair<int, int>> matches;  // 存储匹配的起始位置和长度
    
    while ((pos = result.indexOf(keyword, pos, Qt::CaseInsensitive)) != -1) {
        matches.append(qMakePair(pos, keyword.length()));
        pos += keyword.length();
    }
    
    // 如果没有匹配，直接返回
    if (matches.isEmpty()) {
        return QString("<span style='color:%1'>%2</span>")
            .arg(colorName)
            .arg(text.toHtmlEscaped());
    }
    
    // 从后往前替换，避免位置偏移
    QString html;
    int lastPos = 0;
    
    for (int i = 0; i < matches.size(); ++i) {
        int matchPos = matches[i].first;
        int matchLen = matches[i].second;
        
        // 添加匹配前的文本
        if (matchPos > lastPos) {
            html += QString("<span style='color:%1'>%2</span>")
                .arg(colorName)
                .arg(result.mid(lastPos, matchPos - lastPos).toHtmlEscaped());
        }
        
        // 添加高亮的匹配文本
        html += QString("<span style='background-color:#FFFF99; color:%1'>%2</span>")
            .arg(colorName)
            .arg(result.mid(matchPos, matchLen).toHtmlEscaped());
        
        lastPos = matchPos + matchLen;
    }
    
    // 添加剩余文本
    if (lastPos < result.length()) {
        html += QString("<span style='color:%1'>%2</span>")
            .arg(colorName)
            .arg(result.mid(lastPos).toHtmlEscaped());
    }
    
    return html;
}
