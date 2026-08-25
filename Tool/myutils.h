#ifndef MYUTILS_H
#define MYUTILS_H
#include <QToolButton>
#include <QObject>
#include <QLabel>
class MyUtils
{

public:
    MyUtils();
    static void setIcons(QToolButton* button, const QString& text, const QString& img_path);

    static void setBack(QWidget* widget);

    static void setLabel(QLabel *label,int font_size,bool is_bold);

    static void setLabelImg(QLabel *label,const QString &img_path,int size=40);
};

#endif // MYUTILS_H
