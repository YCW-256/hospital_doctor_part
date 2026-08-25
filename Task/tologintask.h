#ifndef TOLOGINTASK_H
#define TOLOGINTASK_H

#include <QObject>
#include "businesstask.h"

class ToLoginTask : public BusinessTask
{
    Q_OBJECT
public:
    explicit ToLoginTask(QObject *parent = nullptr);

    explicit ToLoginTask(const QString name,const QString pwd,const QString ver_card,QObject *parent = nullptr);

    void execute();

    QByteArray send_data;

    int len;
private:
    QString name;
    QString pwd;
    QString ver_card;


signals:
    void to_login_ok(const QByteArray data,int len);
};

#endif // TOLOGINTASK_H
