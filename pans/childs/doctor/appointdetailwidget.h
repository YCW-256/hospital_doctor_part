#ifndef APPOINTDETAILWIDGET_H
#define APPOINTDETAILWIDGET_H

#include <QDialog>
#include <QString>

class QLabel;
class QTextEdit;
class QPushButton;
class RecordPdf;

// 预约卡片点击后弹出的“接诊详情”弹窗（普通医生端 AppointWidget 专用，代码式 UI，无 .ui）。
// 左右布局：左侧（保持原有外观）展示该次预约的病人姓名、时间（日期与时段），医生在此录入 诊断 / 处方；
// 右侧新增舌苔图片位 QLabel m_tongueLabel（暂无图片，默认黑底占位，后续接图后 setPixmap 显示）。
// 底部“返回”直接关闭，“完成”先弹警告二次确认，确认后把该次预约的全量信息
// （meet_id / doctor_id / patient_id / doctor_name / patient_name / 诊断 / 处方）qDebug 打印，
// 再 accept()；调用方（AppointWidget）读 diagnosis()/treatPlan() 打包 DOCTOR_SET_RECORD 发出。
// “一键导出”按钮为开关：点一下文字变“取消”（黑白反色），再点恢复；
// 开启状态下点“完成”会顺带把本次诊疗记录导出成 PDF（RecordPdf = 离屏 WebEngine 渲染病历 HTML）。
// **导出是异步的**：开启导出时“完成”不再立刻 accept()，而是禁用按钮 → 导出 → 回调里弹提示再 accept()。
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

    // “一键导出”开关当前是否开启（开启=点“完成”时导出 PDF）。默认关闭。
    bool exportPdfEnabled() const;

    // 本次导出得到的 PDF 绝对路径；未开启导出或导出失败时为空串
    QString exportedFilePath() const;

    // 导出进行中（离屏渲染 + printToPdf 是异步的）时屏蔽 返回/关闭，
    // 免得把弹窗关掉、页面被销毁导致导出半途而废
    void reject() override;

private slots:
    void onDone();   // “完成”：先弹警告二次确认，确认后打印全量信息、（开了导出则）导出 PDF，再 accept()

private:
    void buildUi();          // 组装界面（只建一次）
    void printAll() const;   // 打印弹窗中的全量信息（qDebug）
    void updateExportBtn();  // 按开关状态刷新“一键导出/取消”的文字与配色
    void startExport();      // 用当前填写内容调 RecordPdf 导出病历 PDF（**异步**：结束后在回调里弹提示再 accept）

    MeetRecord m_rec;        // 构造时传入的该次预约信息（快照，弹窗期间不依赖 CData）
    QString    m_exportedPath;   // 本次导出的 PDF 路径（空 = 未导出/导出失败）
    RecordPdf *m_exporter;       // 导出器（异步；挂在弹窗下，随弹窗销毁）
    bool       m_exporting;      // 导出进行中（期间不接受返回/关闭）

    // ---- 控件 ----
    QLabel      *m_patientLabel;   // 患者姓名
    QLabel      *m_timeLabel;      // 时间（日期与时段）
    QLabel      *m_extraLabel;     // 医生 / 预约号 等次要信息
    QLabel      *m_tongueLabel;    // 右侧舌苔图片位（暂无图片，黑底占位）
    QTextEdit   *m_diagnosisEdit;      // 诊断
    QTextEdit   *m_prescriptionEdit;   // 处方
    QPushButton *m_exportBtn;      // 一键导出（可选中开关，选中时显示“取消”）
    QPushButton *m_backBtn;        // 返回
    QPushButton *m_doneBtn;        // 完成
};

#endif // APPOINTDETAILWIDGET_H
