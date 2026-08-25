#ifndef BUSINESSTASK_H
#define BUSINESSTASK_H

#include <QObject>
#include "MyTcp/protecol.h"
class BusinessTask : public QObject
{
    Q_OBJECT
public:
    explicit BusinessTask(QObject *parent = nullptr);

    BusinessTask(int len,const QByteArray &data, QObject *parent);

    virtual void execute() = 0;
protected:
    int len;
    QByteArray data;

signals:
};

#endif // BUSINESSTASK_H
