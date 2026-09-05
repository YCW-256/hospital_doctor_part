#ifndef COPYSCHEDULE_H
#define COPYSCHEDULE_H

#include <QDialog>
#include <QDate>
#include <QString>

class QLabel;
class QSpinBox;
class QPushButton;

// 批量复制排班弹窗（管理员排班页 DoctorOrder 专用）。
// 作用：把当前展示周的整周排班，复制到接下来的连续 N 周。
// 用户用 QSpinBox 选“复制到第 N 周”（1..26），界面实时显示 源周 / 下一周 / 最后一周 的日期。
// 点“确认”→ 先弹二级确认框（提示会整周覆盖目标周）→ 确认后 accept()，
// 由调用方 DoctorOrder 读 copyWeeks()，逐周打包 REPIX_GUARD 发送（每周一个包，QTimer 节奏）。
class CopyScheduleDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CopyScheduleDialog(const QDate &srcMonday,
                                const QString &department,
                                QWidget *parent = nullptr);

    // 返回要复制的连续周数 N（>=1，源周之后的下 N 周）
    int copyWeeks() const;

private slots:
    void refreshTarget();    // N 变化时刷新目标周日期文字
    void onConfirm();        // 确认按钮：先二级确认，再 accept()

private:
    void buildUi();                                   // 组装界面（只建一次）
    // “周一 yyyy年M月d日 ～ 周日 M月d日”
    static QString rangeText(const QDate &monday);

    QDate   m_srcMonday;   // 源排班周的周一（= DoctorOrder::m_monday）
    QString m_department;  // 当前科室（仅作展示）
    int     m_weeks;       // 当前选择的连续复制周数 N

    QLabel      *m_srcLabel;    // 源排班周日期
    QSpinBox    *m_weeksSpin;   // 复制到第 N 周
    QLabel      *m_targetLabel; // 目标周日期汇总
    QPushButton *m_okBtn;       // 确认
    QPushButton *m_cancelBtn;   // 取消
};

#endif // COPYSCHEDULE_H
