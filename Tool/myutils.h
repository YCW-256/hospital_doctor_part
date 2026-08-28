#ifndef MYUTILS_H
#define MYUTILS_H
#include <QToolButton>
#include <QObject>
#include <QLabel>
#include "QDate"
#include "../MyTcp/protecol.h"
#include "string.h"
class MyUtils
{

public:
    MyUtils();
    static void setIcons(QToolButton* button, const QString& text, const QString& img_path);

    static void setBack(QWidget* widget,QString color1="#D4E9F9",QString color2="#FFFFFF");


    //#E8F2F6

    static void setLabel(QLabel *label,int font_size,bool is_bold);

    static void setLabelImg(QLabel *label,const QString &img_path,int size=40);
    //根据日期返回当周
    static void weekAndDate(QLabel *labels[],QDate=QDate::currentDate());

    static void weekAndDate(QLabel *labels[],GUARD_REPIX_T info[][7] ,QDate=QDate::currentDate());
};

#endif // MYUTILS_H
