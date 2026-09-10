#include "getmedicalrecordtask.h"
#include "../MyTcp/protecol.h"
#include "../MyTcp/cdata.h"

// char[] 线上字段不一定带结束符：按“遇 \0 或写满”取有效长度再转 QString，避免越界读
static QString toStr(const char *buf, int maxLen)
{
    int n = 0;
    while(n < maxLen && buf[n] != '\0')
        ++n;
    return QString::fromUtf8(buf, n);
}

GetMedicalRecordTask::GetMedicalRecordTask(QObject *parent)
    : BusinessTask{parent}
{

}

GetMedicalRecordTask::GetMedicalRecordTask(int len, QByteArray &data, QObject *parent)
    :BusinessTask(len,data,parent)
{

}

void GetMedicalRecordTask::execute()
{
    GET_MEDICAL_RECORD_RESP resp;
    memset(&resp,0,sizeof(resp));
    // 防护：只按实际收到的字节数拷贝，服务端多回/少回都不越界（同 GetGuardTask 的拷法，但取小）
    int cp = this->len < (int)sizeof(resp) ? this->len : (int)sizeof(resp);
    memcpy(&resp, this->data.constData(), cp);

    // count 双向夹紧：既不能超过数组容量，也不能超过本包实际收到的字节能装下的条数
    // （服务端 count 写大了的话，多出来的 items[] 是 memset 出来的全 0，存进 CData 只会是脏行）
    int fitInPack = (int)((this->len - 2 * (int)sizeof(int)) / (int)sizeof(MEDICAL_RECORD_LIST_ITEM));
    int count = resp.count;
    if(count < 0) count = 0;
    if(count > MEDICAL_RECORD_MAX_ITEMS) count = MEDICAL_RECORD_MAX_ITEMS;
    if(count > fitInPack) count = fitInPack < 0 ? 0 : fitInPack;

    // 一次查询 = 一份新结果，整体覆盖
    CData::medical_record_list.clear();
    CData::medical_record_list.resize(count);
    CData::medical_record_total = resp.total;

    qDebug() << "========== 收到病历列表响应 ==========";
    qDebug() << "总条数:" << resp.total << "本包条数:" << resp.count << "(实际采纳" << count << ")";
    for (int i = 0; i < count; ++i) {
        const MEDICAL_RECORD_LIST_ITEM &it = resp.items[i];
        MEDICAL_RECORD_INFO &info = CData::medical_record_list[i];
        info.record_id    = it.record_id;
        info.patient_id   = it.patient_id;
        info.state        = it.state;
        info.patient_name = toStr(it.patient_name, sizeof(it.patient_name));
        info.record_time  = toStr(it.record_time,  sizeof(it.record_time));
        info.main_symptom = toStr(it.main_symptom, sizeof(it.main_symptom));

        qDebug() << "[" << i << "]"
                 << "病历编号:" << info.record_id
                 << "患者编号:" << info.patient_id
                 << "姓名:" << info.patient_name
                 << "时间:" << info.record_time
                 << "症状:" << info.main_symptom
                 << "状态:" << (info.state == 0 ? "正常" : "作废");
    }
    qDebug() << "======================================";
}
