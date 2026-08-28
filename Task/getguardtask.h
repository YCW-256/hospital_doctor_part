#ifndef GETGUARDTASK_H
#define GETGUARDTASK_H

#include "businesstask.h"

class GetGuardTask : public BusinessTask
{
public:
    explicit GetGuardTask(QObject *parent = nullptr);

    GetGuardTask(int len,QByteArray &data ,QObject *parent);

    void execute();

};

#endif // GETGUARDTASK_H
