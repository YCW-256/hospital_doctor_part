#ifndef READUTIL_H
#define READUTIL_H
#include <iostream>
#include <QString>
#include <QWidget>
#include <QFile>
using namespace std;
class ReadUtil
{
public:
    ReadUtil();

    static bool readQssFile(const QString &filePath, QString &outQss);

    static bool setWidgetQss(QWidget *widget, const QString &qssPath);
};

#endif // READUTIL_H
