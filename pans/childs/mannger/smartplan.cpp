#include "smartplan.h"
#include <QVector>
#include <climits>

namespace {
// 排班表固定：一周 7 天 × 3 时段
const int kPeriods = 3;
const int kDays = 7;
}

SmartPlanOut runPlan(const SmartPlanIn &in, int algoId)
{
    switch (algoId) {
    case AlgBalancedWeek: return planBalancedWeek(in);
    default: return planBalancedWeek(in);
    }
}

SmartPlanOut planBalancedWeek(const SmartPlanIn &in)
{
    SmartPlanOut out;
    out.ok = true;
    out.unfilled = 0;
    for (int k = 0; k < kPeriods; ++k)
        for (int d = 0; d < kDays; ++d)
            out.assign[k][d] = -1;

    const int nDoc = in.doctorIds.size();
    if (nDoc <= 0) {
        out.ok = false;
        out.warn = QStringLiteral("当前科室没有医生，无法自动排班。");
        return out;
    }

    int cap = in.maxShiftPerDoctorPerDay;
    if (cap < 1)
        cap = 1;
    if (cap > kPeriods)
        cap = kPeriods;

    // 班次统计（保留模式下把“已排的本科室医生”也计入公平与每日上限）
    QVector<int> load(nDoc, 0);
    QVector<QVector<int> > perDay(nDoc);
    for (int i = 0; i < nDoc; ++i)
        perDay[i].fill(0, kDays);

    if (in.keepExisting) {
        for (int k = 0; k < kPeriods; ++k) {
            for (int d = 0; d < kDays; ++d) {
                int e = in.existing[k][d];
                if (e >= 0 && e < nDoc) {
                    out.assign[k][d] = e;      // 本科室医生已值班：保留原样
                    ++load[e];
                    ++perDay[e][d];
                }
                else if (e == -3) {
                    out.assign[k][d] = -3;     // 外人占用：保留原样
                }
                // e == -1 的空格留待下方填充
            }
        }
    }
    // keepExisting == false 即“覆盖”：上面没复制任何现有排班，全部重新排。

    // 贪心填充：能排的格子尽量排满，且各医生本周班次尽量平均。
    // 每轮先找“可选医生最少”的格子（最难排的先排，避免最后没人可选），
    // 再在可选医生里挑“已排班次最少”的，班次相同取序号小的（结果确定、可复现）。
    bool anyFill = true;
    while (anyFill) {
        anyFill = false;
        int bestK = -1, bestD = -1, bestDeg = INT_MAX, bestDoc = -1;
        for (int k = 0; k < kPeriods; ++k) {
            for (int d = 0; d < kDays; ++d) {
                if (in.dayDisabled[d] || out.assign[k][d] != -1)
                    continue;
                int chosenDoc = -1;
                int minLoad = INT_MAX;
                int deg = 0;
                for (int i = 0; i < nDoc; ++i) {
                    if (i < in.doctorDayBlocked.size() && d < in.doctorDayBlocked[i].size()
                        && in.doctorDayBlocked[i][d])
                        continue;
                    if (perDay[i][d] >= cap)
                        continue;
                    ++deg;
                    if (load[i] < minLoad) {
                        minLoad = load[i];
                        chosenDoc = i;
                    }
                }
                if (deg == 0)
                    continue;                    // 这格当前无人可用：放弃
                if (deg < bestDeg) {
                    bestDeg = deg;
                    bestK = k;
                    bestD = d;
                    bestDoc = chosenDoc;
                    anyFill = true;
                }
            }
        }
        if (!anyFill)
            break;
        out.assign[bestK][bestD] = bestDoc;
        ++load[bestDoc];
        ++perDay[bestDoc][bestD];
        anyFill = true;
    }

    // 汇总：非整日禁排却仍为空（没排满）的格
    int unfilled = 0;
    for (int k = 0; k < kPeriods; ++k)
        for (int d = 0; d < kDays; ++d)
            if (!in.dayDisabled[d] && out.assign[k][d] == -1)
                ++unfilled;
    out.unfilled = unfilled;
    if (unfilled > 0)
        out.warn = QStringLiteral("有 %1 个时段因当日无可用医生（禁排/每日上限）未能排满。").arg(unfilled);

    return out;
}
