#include "getguardtask.h"
#include "../MyTcp/protecol.h"
#include "../MyTcp/cdata.h"
GetGuardTask::GetGuardTask(QObject *parent)
    : BusinessTask{parent}
{


}

GetGuardTask::GetGuardTask(int len, QByteArray &data, QObject *parent)
    :BusinessTask(len,data,parent)
{

}

void GetGuardTask::execute()
{
    // 2. 拷贝数据到全局变量（假设 m_get_cards 是 GET_GUARD_RESP 类型）
    memcpy(&CData::m_get_cards, this->data, this->len);

    // 3. 打印所有字段（调试用）
    qDebug() << "========== 收到排班响应数据 ==========";
    // for (int i = 0; i < 3; ++i) {
    //     for (int j = 0; j < 7; ++j) {
    //         const GUARD_REPIX_T& g = CData::m_get_cards.guards[i][j];
    //         qDebug() << "时段[" << i << "][" << j << "]"
    //                  << "姓名:" << QString::fromUtf8(g.name)
    //                  << "日期:" << QString::fromUtf8(g.date)
    //                  << "时段编号:" << g.time
    //                  << "空闲:" << (g.isfree ? "是" : "否")
    //                  << "科室:" << QString::fromUtf8(g.depart);
    //     }
    // }
    // qDebug() << "========================================";

}
