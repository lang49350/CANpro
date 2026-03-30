#ifndef SIMPLEXLSXWRITER_H
#define SIMPLEXLSXWRITER_H

#include <QString>
#include <QVector>
#include <QStringList>

/**
 * @brief 简单的XLSX文件写入器
 * 
 * 生成Excel 2003 XML格式文件
 * Excel可以直接打开这种格式
 */
class SimpleXlsxWriter
{
public:
    SimpleXlsxWriter();
    ~SimpleXlsxWriter();
    
    // 添加表头
    void addHeader(const QStringList &headers);
    
    // 添加一行数据
    void addRow(const QStringList &row);
    
    // 保存到文件
    bool save(const QString &filename);
    
    // 清空数据
    void clear();
    
private:
    QStringList m_headers;
    QVector<QStringList> m_rows;
    
    // 转义XML特殊字符
    QString escapeXml(const QString &text) const;
};

#endif // SIMPLEXLSXWRITER_H
