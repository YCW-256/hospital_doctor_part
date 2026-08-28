#ifndef GETDEPARTMENTDOCTORTASK_H
#define GETDEPARTMENTDOCTORTASK_H

#include "businesstask.h"

class GetDepartmentDoctorTask : public BusinessTask
{
public:
    explicit GetDepartmentDoctorTask(QObject *parent = nullptr);

    GetDepartmentDoctorTask(int len,QByteArray &data ,QObject *parent);

    void execute();
};

#endif // GETDEPARTMENTDOCTORTASK_H
