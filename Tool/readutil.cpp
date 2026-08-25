#include "readutil.h"
#include <QFile>
#include <QDebug>

ReadUtil::ReadUtil() {}

bool ReadUtil::readQssFile(const QString &filePath, QString &outQss)
{
    outQss.clear();
    QFile file(filePath);
    // 只读，不要 QIODevice::Text，保留原始字节
    if (!file.open(QIODevice::ReadOnly))
    {
        qWarning() << "打开QSS失败:" << filePath << file.errorString();
        return false;
    }
    QByteArray rawData = file.readAll();
    file.close();

    // 移除 UTF-8 BOM 3字节头部（元凶！）
    if (rawData.startsWith("\xEF\xBB\xBF"))
    {
        rawData = rawData.mid(3);
    }

    // Qt6 标准方式：直接 fromUtf8
    outQss = QString::fromUtf8(rawData);
    return true;
}

bool ReadUtil::setWidgetQss(QWidget *widget, const QString &qssPath)
{
    if (!widget) return false;
    QString qss;
    if (!readQssFile(qssPath, qss))
        return false;
    widget->setStyleSheet(qss);
    return true;
}