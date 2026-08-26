#ifndef GETINFOTASK_H
#define GETINFOTASK_H

#include "businesstask.h"

class GetInfoTask : public BusinessTask
{
public:
    explicit GetInfoTask(QObject *parent = nullptr);

    GetInfoTask(int len,QByteArray &data ,QObject *parent);

    void execute();
};

#endif // GETINFOTASK_H
