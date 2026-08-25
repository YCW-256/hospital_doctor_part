#include "businesstask.h"

BusinessTask::BusinessTask(QObject *parent)
    : QObject{parent}
{}

BusinessTask::BusinessTask(int len, const QByteArray &data, QObject *parent)
    : QObject(parent)
    , len(len)
    , data(data)
{

}
