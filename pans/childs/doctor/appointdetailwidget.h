#ifndef APPOINTDETAILWIDGET_H
#define APPOINTDETAILWIDGET_H

#include <QDialog>
#include <QString>
#include <QImage>

class QLabel;
class QTextEdit;
class QPushButton;
class QTimer;
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
// 舌苔图片位下方另有“获得图片”按钮：点击发 to_get_tongue_img 信号（医生 id / 患者 id / 日期），
// 由调用方 AppointWidget 打包 GET_TONGUE_IMG 发服务器；回包（分片）由 SocketLink 那边的
// GetTongueImgTask 拼成 QImage 存进 CData，再经 AppointWidget::flush_tongue_img 调回本弹窗的
// setTongueImage() 显示。服务端没有该次就诊的图片时**不回包**，靠 5 秒单次定时器兜底提示可重试。
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

    // 收到服务端回包后由 AppointWidget 调用：把舌苔图片显示到 m_tongueLabel。
    // patientId 是在回包里带的患者编号 —— 不是本次就诊的患者就忽略（免得串到别人的图上）；
    // img 为空 / 认领不到图片时把图片位恢复成“未找到”，提示可重试。
    void setTongueImage(const QImage &img, int patientId);

    // 导出进行中（离屏渲染 + printToPdf 是异步的）时屏蔽 返回/关闭，
    // 免得把弹窗关掉、页面被销毁导致导出半途而废
    void reject() override;

signals:
    // “获得图片”：请求本次就诊的舌苔图片（doctorId/patientId/date 由调用方打包 GET_TONGUE_IMG）
    void to_get_tongue_img(int doctorId, int patientId, const QString &date);

private slots:
    void onDone();        // “完成”：先弹警告二次确认，确认后打印全量信息、（开了导出则）导出 PDF，再 accept()
    void onGetTongueImg();// “获得图片”：取当前预约的 医生 id / 患者 id / 日期 发信号（回包暂不接收）

private:
    void buildUi();          // 组装界面（只建一次）
    void printAll() const;   // 打印弹窗中的全量信息（qDebug）
    void updateExportBtn();  // 按开关状态刷新“一键导出/取消”的文字与配色
    void startExport();      // 用当前填写内容调 RecordPdf 导出病历 PDF（**异步**：结束后在回调里弹提示再 accept）
    QString recordDate() const;  // 从 m_rec.time 里取日期部分 yyyy-MM-dd（取不到就用今天）
    void showTonguePixmap();     // 把 m_rec 对应的 CData::tongue_image 画进图片位（缓存命中时用）
    void clearTonguePlaceholder(const QString &tip);  // 图片位回到黑底占位态并给出提示文字

    MeetRecord m_rec;        // 构造时传入的该次预约信息（快照，弹窗期间不依赖 CData）
    QString    m_exportedPath;   // 本次导出的 PDF 路径（空 = 未导出/导出失败）
    RecordPdf *m_exporter;       // 导出器（异步；挂在弹窗下，随弹窗销毁）
    bool       m_exporting;      // 导出进行中（期间不接受返回/关闭）
    QTimer    *m_imgTimer;       // “获得图片”的等待超时（服务端找不到图时不回包，到点提示可重试）
    bool       m_imgWaiting;     // 已发出图片请求、正在等服务端回包

    // ---- 控件 ----
    QLabel      *m_patientLabel;   // 患者姓名
    QLabel      *m_timeLabel;      // 时间（日期与时段）
    QLabel      *m_extraLabel;     // 医生 / 预约号 等次要信息
    QLabel      *m_tongueLabel;    // 右侧舌苔图片位（无图时黑底占位；有图 setPixmap 等比显示）
    QPushButton *m_getImgBtn;      // “获得图片”（舌苔图片位下方，向服务端请求该次就诊的舌苔照）
    QTextEdit   *m_diagnosisEdit;      // 诊断
    QTextEdit   *m_prescriptionEdit;   // 处方
    QPushButton *m_exportBtn;      // 一键导出（可选中开关，选中时显示“取消”）
    QPushButton *m_backBtn;        // 返回
    QPushButton *m_doneBtn;        // 完成
};

#endif // APPOINTDETAILWIDGET_H
