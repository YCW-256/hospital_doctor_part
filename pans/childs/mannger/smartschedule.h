#ifndef SMARTSCHEDULE_H
#define SMARTSCHEDULE_H

#include <QDialog>
#include <QDate>
#include <QVector>
#include <QString>
#include "../../../MyTcp/protecol.h"

class QLabel;
class QRadioButton;
class QSpinBox;
class QCheckBox;
class QPushButton;
class QGridLayout;
class QVBoxLayout;
class QWidget;

// smartplan.h 里的结果结构（renderPreview 按 const& 用，前向声明即可）
struct SmartPlanOut;

// 智能排班对话框（管理员 DoctorOrder 专用）。
// 把“当前科室医生”排进“当前展示周”的 21 个时段(7天×3段，一格一人)。
// 进入时【不发任何服务器请求】，周/科室/医生/现有排班全由 DoctorOrder 快照传入。
// 提供：覆盖当前已排班 / 保留当前已排班 两种模式；每人每天最多班次；
//      整日禁排 与 医生×星期几禁排（可叠加）；点“生成方案”出预览，点“应用并保存”
//      把最终一周排班写回 DoctorOrder 并触发一次 REPIX_GUARD。
class SmartScheduleDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SmartScheduleDialog(const QDate &monday,
                                 const QString &department,
                                 const QVector<QString> &docNames,
                                 const QVector<int> &docIds,
                                 const GUARD_REPIX_T oldInfo[3][7],
                                 QWidget *parent = nullptr);

    // 应用后把生成的一周排班（与 DoctorOrder::m_info 同构）拷给调用方
    void resultPlan(GUARD_REPIX_T out[3][7]) const;

private slots:
    void onGenerate();   // 按当前约束算一遍并刷新预览
    void onApply();      // 有生成结果才可点：简单确认后 accept()，由 DoctorOrder 发包

private:
    void buildUi();
    void clearPreview();    // 清掉旧预览表格
    void renderPreview(const SmartPlanOut &out);   // 画 3×7 预览表格 + 汇总文字

    QString dayText(int d) const;      // “周一\nM月d日”
    int existingIndexOf(const GUARD_REPIX_T &cell) const;  // 旧排班格->医生序号/-1/-3
    static QString doctorNameOf(const GUARD_REPIX_T &cell); // 读格内值班医生名(trimmed)

    // ---- 输入快照（构造时由 DoctorOrder 传入，不重新联网） ----
    QDate           m_monday;
    QString         m_department;
    QVector<QString> m_docNames;
    QVector<int>     m_docIds;
    GUARD_REPIX_T   m_oldInfo[3][7];

    // ---- 最近一次“生成”的结果 ----
    GUARD_REPIX_T   m_result[3][7];
    bool            m_generated = false;
    QString         m_resultWarn;

    // ---- 控件 ----
    QRadioButton *m_overwriteRadio;
    QRadioButton *m_keepRadio;
    QSpinBox     *m_capSpin;
    QCheckBox    *m_dayDisable[7];        // 某天整日禁排
    QWidget      *m_matrixContent;        // 医生×星期几矩阵（放滚动区）
    QGridLayout  *m_matrixGrid;
    QVector<QCheckBox*> m_docChecks;      // 序号 = 医生*7+星期
    QLabel       *m_summaryLabel;
    QWidget      *m_tableHolder;          // 预览表格容器（每次生成重建）
    QVBoxLayout  *m_tableLay;
    QPushButton  *m_genBtn;
    QPushButton  *m_applyBtn;
    QPushButton  *m_cancelBtn;
};

#endif // SMARTSCHEDULE_H
