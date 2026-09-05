#ifndef SMARTPLAN_H
#define SMARTPLAN_H

#include <QString>
#include <QVector>

// =====================================================================
// 智能排班算法模块（纯算法，与 UI / 网络协议解耦）。
// 输入输出统一走“医生序号”（0..doctorCount-1），不出现姓名/协议结构体，
// 方便将来加新算法：在 SmartAlgId 加枚举、写一个同签名的函数、
// 并在 runPlan() 的 switch 里分发即可。
//
// 一周固定 7 天 × 3 时段（上午/下午/晚上），格子用 [period][day] 表示，
// 与排班页本地模型 m_info[k][d]（k=时段, d=星期）方向一致。
// =====================================================================

struct SmartPlanIn {
    QVector<int> doctorIds;                    // 本科室医生 id（只用到个数与顺序）
    bool dayDisabled[7];                       // true=该星期几整天禁排（整周排不了人）
    QVector<QVector<bool> > doctorDayBlocked;  // [doctor][day] true=该医生该星期几禁排
    int  maxShiftPerDoctorPerDay = 1;          // 每人每天最多排班数（1..时段数）
    bool keepExisting = false;                 // true=保留当前已排(只补空格)；false=覆盖整周重排

    // existing[k][d]：本周当前排班，取值约定：
    //   医生序号(>=0) 本科室某医生已值班； -1 空格(待排)；
    //   -3 该格被“不在本科室医生表里”的人占用（保留模式下原样保留、不参与重排）
    int existing[3][7];
};

struct SmartPlanOut {
    int assign[3][7];                          // 结果，取值约定同 existing
    bool ok = true;
    int  unfilled = 0;                         // 想排满却仍空的格数（能排的都已尽量排）
    QString warn;                              // 提示文本（无则空）
};

// 算法 id（扩展点：加新算法在此加枚举值并在 runPlan 分发）
enum SmartAlgId { AlgBalancedWeek = 0 };       // 本周尽量平均

// 统一入口：按算法 id 分发展计划
SmartPlanOut runPlan(const SmartPlanIn &in, int algoId = AlgBalancedWeek);

// 具体算法①：本周尽量平均。
// 在“每人每天最多班”等约束下，把能排的格子填满，
// 且让各医生本周班次尽量平均（优先把剩余格子分给班次少的人）。
SmartPlanOut planBalancedWeek(const SmartPlanIn &in);

#endif // SMARTPLAN_H
