#ifndef BUSINESSTASK_H
#define BUSINESSTASK_H
#include <QDebug>
#include <QObject>
#include "MyTcp/protecol.h"
#include "MyTcp/cdata.h"
#include <String.h>
using namespace std;
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
