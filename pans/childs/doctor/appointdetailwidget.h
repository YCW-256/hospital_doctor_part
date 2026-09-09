#ifndef APPOINTDETAILWIDGET_H
#define APPOINTDETAILWIDGET_H

#include <QDialog>
#include <QString>

class QLabel;
class QTextEdit;
class QPushButton;

// 预约卡片点击后弹出的“接诊详情”弹窗（普通医生端 AppointWidget 专用，代码式 UI，无 .ui）。
// 左右布局：左侧（保持原有外观）展示该次预约的病人姓名、时间（日期与时段），医生在此录入 诊断 / 处方；
// 右侧新增舌苔图片位 QLabel m_tongueLabel（暂无图片，默认黑底占位，后续接图后 setPixmap 显示）。
// 底部“返回”直接关闭，“完成”先弹警告二次确认，确认后把该次预约的全量信息
// （meet_id / doctor_id / patient_id / doctor_name / patient_name / 诊断 / 处方）qDebug 打印，
// 再 accept()；调用方（AppointWidget）读 diagnosis()/treatPlan() 打包 DOCTOR_SET_RECORD 发出。
struct MeetRecord {
    int     meet_id    = -1;  // 预约/挂号号（取自 DOCTOR_APP_RESP.meet_id）
    int     doctor_id  = -1;  // 接诊医生 id（登录医生 CData::m_id，查的是自己的预约）
    int     patient_id = -1;  // 患者 id（DOCTOR_APP_RESP.patient_id）
    QString doctor_name;      // 接诊医生名（取自 DOCTOR_APP_RESP.DoctorName）
    QString patient_name;     // 患者名（取自 DOCTOR_APP_RESP.PatientName）
    QString time;             // 就诊时间（日期与时段，服务端下发的原始时间串）
};

class AppointDetailWidget : public QDialog
{
    Q_OBJECT

public:
    explicit AppointDetailWidget(const MeetRecord &rec, QWidget *parent = nullptr);

    // 完成（exec 返回 Accepted）后由调用方读取，用于打包 SET_RECORD_REQ
    QString diagnosis() const;   // 诊断文本（已 trim）
    QString treatPlan() const;   // 处方/治疗方案文本（已 trim）

private slots:
    void onDone();   // “完成”：先弹警告二次确认，确认后打印全量信息并 accept()

private:
    void buildUi();          // 组装界面（只建一次）
    void printAll() const;   // 打印弹窗中的全量信息（qDebug）

    MeetRecord m_rec;        // 构造时传入的该次预约信息（快照，弹窗期间不依赖 CData）

    // ---- 控件 ----
    QLabel      *m_patientLabel;   // 患者姓名
    QLabel      *m_timeLabel;      // 时间（日期与时段）
    QLabel      *m_extraLabel;     // 医生 / 预约号 等次要信息
    QLabel      *m_tongueLabel;    // 右侧舌苔图片位（暂无图片，黑底占位）
    QTextEdit   *m_diagnosisEdit;      // 诊断
    QTextEdit   *m_prescriptionEdit;   // 处方
    QPushButton *m_backBtn;        // 返回
    QPushButton *m_doneBtn;        // 完成
};

#endif // APPOINTDETAILWIDGET_H
