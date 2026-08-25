#ifndef LOGINTASK_H
#define LOGINTASK_H

#include <QObject>
#include "businesstask.h"
class LoginTask : public BusinessTask
{
    Q_OBJECT
public:
    explicit LoginTask(QObject *parent = nullptr);

    LoginTask(int len,QByteArray &data ,QObject *parent);

    void execute();

    bool is_success;

signals:
};

#endif // LOGINTASK_H
