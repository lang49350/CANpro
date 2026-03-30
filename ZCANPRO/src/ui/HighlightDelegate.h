/**
 * @file HighlightDelegate.h
 * @brief 高亮显示委托 - 支持文本级别的高亮
 * @author CANAnalyzerPro Team
 * @date 2024-02-10
 */

#ifndef HIGHLIGHTDELEGATE_H
#define HIGHLIGHTDELEGATE_H

#include <QStyledItemDelegate>
#include <QPainter>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QApplication>

/**
 * @brief 高亮显示委托
 * 
 * 功能：
 * - 支持在单元格内部分文本高亮
 * - 使用HTML富文本渲染
 * - 保持原有的文本颜色和对齐方式
 */
class HighlightDelegate : public QStyledItemDelegate
{
    Q_OBJECT
    
public:
    explicit HighlightDelegate(QObject *parent = nullptr);
    
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;
    
    /**
     * @brief 设置高亮关键词
     * @param column 列索引
     * @param keyword 关键词
     */
    void setHighlightKeyword(int column, const QString &keyword);
    
    /**
     * @brief 清除所有高亮关键词
     */
    void clearHighlightKeywords();
    
private:
    /**
     * @brief 生成高亮HTML
     * @param text 原始文本
     * @param keyword 关键词
     * @param textColor 文本颜色
     * @return HTML字符串
     */
    QString generateHighlightHtml(const QString &text, const QString &keyword, const QColor &textColor) const;
    
    QMap<int, QString> m_highlightKeywords;  // 列索引 -> 关键词
};

#endif // HIGHLIGHTDELEGATE_H
