#include "SimpleXlsxWriter.h"
#include <QFile>
#include <QTextStream>
#include <QDateTime>

SimpleXlsxWriter::SimpleXlsxWriter()
{
}

SimpleXlsxWriter::~SimpleXlsxWriter()
{
}

void SimpleXlsxWriter::addHeader(const QStringList &headers)
{
    m_headers = headers;
}

void SimpleXlsxWriter::addRow(const QStringList &row)
{
    m_rows.append(row);
}

bool SimpleXlsxWriter::save(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    
    // 写入Excel XML头部
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    out << "<?mso-application progid=\"Excel.Sheet\"?>\n";
    out << "<Workbook xmlns=\"urn:schemas-microsoft-com:office:spreadsheet\"\n";
    out << " xmlns:o=\"urn:schemas-microsoft-com:office:office\"\n";
    out << " xmlns:x=\"urn:schemas-microsoft-com:office:excel\"\n";
    out << " xmlns:ss=\"urn:schemas-microsoft-com:office:spreadsheet\"\n";
    out << " xmlns:html=\"http://www.w3.org/TR/REC-html40\">\n";
    
    // 文档属性
    out << " <DocumentProperties xmlns=\"urn:schemas-microsoft-com:office:office\">\n";
    out << "  <Author>ZCANPRO</Author>\n";
    out << "  <Created>" << QDateTime::currentDateTime().toString(Qt::ISODate) << "</Created>\n";
    out << " </DocumentProperties>\n";
    
    // 样式定义
    out << " <Styles>\n";
    out << "  <Style ss:ID=\"Default\" ss:Name=\"Normal\">\n";
    out << "   <Alignment ss:Vertical=\"Bottom\"/>\n";
    out << "  </Style>\n";
    out << "  <Style ss:ID=\"Header\">\n";
    out << "   <Font ss:Bold=\"1\"/>\n";
    out << "   <Interior ss:Color=\"#CCCCCC\" ss:Pattern=\"Solid\"/>\n";
    out << "   <Borders>\n";
    out << "    <Border ss:Position=\"Bottom\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\"/>\n";
    out << "   </Borders>\n";
    out << "  </Style>\n";
    out << " </Styles>\n";
    
    // 工作表
    out << " <Worksheet ss:Name=\"CAN数据\">\n";
    out << "  <Table>\n";
    
    // 列宽设置
    for (int i = 0; i < m_headers.size(); ++i) {
        out << "   <Column ss:AutoFitWidth=\"1\" ss:Width=\"100\"/>\n";
    }
    
    // 写入表头
    if (!m_headers.isEmpty()) {
        out << "   <Row>\n";
        for (const QString &header : m_headers) {
            out << "    <Cell ss:StyleID=\"Header\"><Data ss:Type=\"String\">";
            out << escapeXml(header);
            out << "</Data></Cell>\n";
        }
        out << "   </Row>\n";
    }
    
    // 写入数据行
    for (const QStringList &row : m_rows) {
        out << "   <Row>\n";
        for (int i = 0; i < row.size(); ++i) {
            // 判断数据类型（数字或字符串）
            bool isNumber = false;
            if (i == 0 || i == 1 || i == 5) {  // 时间戳、通道、长度列是数字
                row[i].toDouble(&isNumber);
            }
            
            out << "    <Cell><Data ss:Type=\"";
            out << (isNumber ? "Number" : "String");
            out << "\">";
            out << escapeXml(row[i]);
            out << "</Data></Cell>\n";
        }
        out << "   </Row>\n";
    }
    
    out << "  </Table>\n";
    out << " </Worksheet>\n";
    out << "</Workbook>\n";
    
    file.close();
    return true;
}

void SimpleXlsxWriter::clear()
{
    m_headers.clear();
    m_rows.clear();
}

QString SimpleXlsxWriter::escapeXml(const QString &text) const
{
    QString result = text;
    result.replace("&", "&amp;");
    result.replace("<", "&lt;");
    result.replace(">", "&gt;");
    result.replace("\"", "&quot;");
    result.replace("'", "&apos;");
    return result;
}
