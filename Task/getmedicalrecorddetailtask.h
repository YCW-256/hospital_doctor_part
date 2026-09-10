#ifndef GETMEDICALRECORDDETAILTASK_H
#define GETMEDICALRECORDDETAILTASK_H

#include "businesstask.h"

// 病历【第二套·详情】：解析 MEDICAL_RECORD_DETAIL_RESP，
// 按 record_id 存进 CData::medical_record_details（来一条存一条，不清空别的）
class GetMedicalRecordDetailTask : public BusinessTask
{
public:
    explicit GetMedicalRecordDetailTask(QObject *parent = nullptr);

    GetMedicalRecordDetailTask(int len,QByteArray &data ,QObject *parent);

    void execute();

};

#endif // GETMEDICALRECORDDETAILTASK_H
