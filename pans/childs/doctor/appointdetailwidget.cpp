#include "appointdetailwidget.h"
#include "../../../Tool/recordpdf.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QTextEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QDate>
#include <QRegularExpression>
#include <QTimer>
#include <QPixmap>
#include <QDebug>
#include "../../../MyTcp/cdata.h"   // 舌苔图片缓存（CData::tongue_image / tongue_image_patient_id）

AppointDetailWidget::AppointDetailWidget(const MeetRecord &rec, QWidget *parent)
    : QDialog(parent)
    , m_rec(rec)
    , m_patientLabel(nullptr)
    , m_timeLabel(nullptr)
    , m_extraLabel(nullptr)
    , m_tongueLabel(nullptr)
    , m_getImgBtn(nullptr)
    , m_diagnosisEdit(nullptr)
    , m_prescriptionEdit(nullptr)
    , m_exporter(nullptr)
    , m_exporting(false)
    , m_imgTimer(nullptr)
    , m_imgWaiting(false)
    , m_exportBtn(nullptr)
    , m_backBtn(nullptr)
    , m_doneBtn(nullptr)
{
    buildUi();
}

QString AppointDetailWidget::diagnosis() const
{
    return m_diagnosisEdit->toPlainText().trimmed();
}

QString AppointDetailWidget::treatPlan() const
{
    return m_prescriptionEdit->toPlainText().trimmed();
}

bool AppointDetailWidget::exportPdfEnabled() const
{
    return m_exportBtn->isChecked();
}

QString AppointDetailWidget::exportedFilePath() const
{
    return m_exportedPath;
}

void AppointDetailWidget::reject()
{
    if (m_exporting) {   // 导出还没落地，先别关（Esc / 右上角 X 都会走到这里）
        qDebug().noquote() << QStringLiteral("正在导出病历 PDF，稍候再关闭…");
        return;
    }
    QDialog::reject();
}

void AppointDetailWidget::buildUi()
{
    setWindowTitle(QStringLiteral("就诊详情"));
    setAttribute(Qt::WA_StyledBackground, true);
    // 注意：父页(AppointWidget 等) 用 MyUtils::setBack 设置的 "QWidget{background: qlineargradient}"
    // 会沿父子链级联进本弹窗。此处用更近的规则把本弹窗整平：窗口自身 #F4FAFF，
    // 内部 QLabel 透明(露出窗口底色)，从而与弹窗背景一致，避免透出外层蓝白渐变。
    setStyleSheet(
        "AppointDetailWidget { background: #F4FAFF; }"
        "AppointDetailWidget QLabel { background: transparent; }");
    setFixedSize(680, 520);

    // 主横向布局：左侧为原接诊信息 + 诊断处方（外观保持不变），右侧放舌苔图片位
    QHBoxLayout *hMain = new QHBoxLayout(this);
    hMain->setContentsMargins(22, 18, 22, 14);
    hMain->setSpacing(16);

    QVBoxLayout *root = new QVBoxLayout;
    root->setSpacing(10);

    QLabel *title = new QLabel(QStringLiteral("接诊信息"), this);
    title->setStyleSheet("border: none; font-size: 18px; font-weight: bold; color: #1E70BF;");
    root->addWidget(title);

    // 患者姓名（大字强调）
    m_patientLabel = new QLabel(this);
    m_patientLabel->setText(QString("患者：%1").arg(
        m_rec.patient_name.isEmpty() ? QStringLiteral("未知") : m_rec.patient_name));
    m_patientLabel->setStyleSheet("border: none; font-size: 17px; font-weight: bold; color: #333333;");
    root->addWidget(m_patientLabel);

    // 时间（日期与时段）
    m_timeLabel = new QLabel(this);
    m_timeLabel->setText(QString("时间：%1").arg(
        m_rec.time.isEmpty() ? QStringLiteral("未提供") : m_rec.time));
    m_timeLabel->setStyleSheet("border: none; font-size: 14px; color: #555555;");
    root->addWidget(m_timeLabel);

    // 次要信息：医生 / 预约号（供核对；缺省值用 - 占位）
    QString extra = QString("医生：%1    预约号：%2")
                        .arg(m_rec.doctor_name.isEmpty() ? QStringLiteral("-") : m_rec.doctor_name)
                        .arg(m_rec.meet_id >= 0 ? QString::number(m_rec.meet_id) : QStringLiteral("-"));
    m_extraLabel = new QLabel(extra, this);
    m_extraLabel->setStyleSheet("border: none; font-size: 13px; color: #888888;");
    root->addWidget(m_extraLabel);

    // 分隔线
    QFrame *line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("color: #B7D4F2; background-color: #B7D4F2; height: 1px; border: none; margin: 4px 0;");
    root->addWidget(line);

    // 诊断输入
    QLabel *diagLabel = new QLabel(QStringLiteral("诊断"), this);
    diagLabel->setStyleSheet("border: none; font-size: 14px; font-weight: bold; color: #333333;");
    root->addWidget(diagLabel);

    m_diagnosisEdit = new QTextEdit(this);
    m_diagnosisEdit->setPlaceholderText(QStringLiteral("请输入诊断……"));
    m_diagnosisEdit->setFixedHeight(90);
    m_diagnosisEdit->setStyleSheet(
        "QTextEdit {"
        "  background: #FFFFFF;"
        "  border: 1px solid #B7D4F2;"
        "  border-radius: 8px;"
        "  font-size: 14px;"
        "  padding: 6px;"
        "}"
        "QTextEdit:focus { border: 1px solid #2F80ED; }");
    root->addWidget(m_diagnosisEdit);

    // 处方输入
    QLabel *presLabel = new QLabel(QStringLiteral("处方"), this);
    presLabel->setStyleSheet("border: none; font-size: 14px; font-weight: bold; color: #333333;");
    root->addWidget(presLabel);

    m_prescriptionEdit = new QTextEdit(this);
    m_prescriptionEdit->setPlaceholderText(QStringLiteral("请输入处方……"));
    m_prescriptionEdit->setFixedHeight(90);
    m_prescriptionEdit->setStyleSheet(
        "QTextEdit {"
        "  background: #FFFFFF;"
        "  border: 1px solid #B7D4F2;"
        "  border-radius: 8px;"
        "  font-size: 14px;"
        "  padding: 6px;"
        "}"
        "QTextEdit:focus { border: 1px solid #2F80ED; }");
    root->addWidget(m_prescriptionEdit);

    root->addStretch();

    // 底部按钮：一键导出 | (弹簧) 返回 | 完成
    QHBoxLayout *btnRow = new QHBoxLayout;
    btnRow->setSpacing(10);

    // “一键导出”：可选中开关，**只用黑白**（不掺其他颜色）。
    // 未开=白底黑框黑字“一键导出”；点击后反过来=黑底白字“取消”（再点恢复）；
    // 黑白反色同样满足“点了按钮会变色”的提示效果，且不引入任何彩色。
    m_exportBtn = new QPushButton(QStringLiteral("一键导出"), this);
    m_exportBtn->setCheckable(true);
    m_exportBtn->setMinimumSize(92, 34);
    m_exportBtn->setStyleSheet(
        "QPushButton{ background:#FFFFFF; color:#000000; border:1px solid #000000; border-radius:6px; font-size:14px; }"
        "QPushButton:checked{ background:#000000; color:#FFFFFF; border:1px solid #000000; }");
    btnRow->addWidget(m_exportBtn);

    btnRow->addStretch();
    m_backBtn = new QPushButton(QStringLiteral("返回"), this);
    m_backBtn->setMinimumSize(92, 34);
    m_backBtn->setStyleSheet(
        "QPushButton{ background:#E6F0FA; color:#333333; border:1px solid #B7D4F2; border-radius:6px; font-size:14px; }"
        "QPushButton:hover{ background:#D6E6F5; }");
    m_doneBtn = new QPushButton(QStringLiteral("完成"), this);
    m_doneBtn->setMinimumSize(92, 34);
    m_doneBtn->setStyleSheet(
        "QPushButton{ background:#2F80ED; color:#FFFFFF; border:none; border-radius:6px; font-size:14px; }"
        "QPushButton:hover{ background:#1E70BF; }");
    btnRow->addWidget(m_backBtn);
    btnRow->addWidget(m_doneBtn);
    root->addLayout(btnRow);

    // 左侧整列（原内容）放入主横向布局，占满剩余宽度，外观不变
    hMain->addLayout(root, 1);

    // ---- 右侧：舌苔图片位 ----
    QVBoxLayout *right = new QVBoxLayout;
    right->setSpacing(8);

    QLabel *tongTitle = new QLabel(QStringLiteral("舌苔图片"), this);
    tongTitle->setStyleSheet("border: none; font-size: 14px; font-weight: bold; color: #333333;");
    right->addWidget(tongTitle);

    m_tongueLabel = new QLabel(this);
    m_tongueLabel->setFixedSize(200, 250);
    // 没有图片时是黑底占位（白字提示），拿到图后 setPixmap 显示舌苔照
    m_tongueLabel->setStyleSheet(
        "QLabel {"
        "  background: #000000;"
        "  border: 1px solid #B7D4F2;"
        "  border-radius: 8px;"
        "  color: #FFFFFF;"
        "  font-size: 13px;"
        "}");
    m_tongueLabel->setAlignment(Qt::AlignCenter);
    m_tongueLabel->setToolTip(QStringLiteral("舌苔图片（暂无）"));
    m_tongueLabel->setWordWrap(true);
    right->addWidget(m_tongueLabel);

    // “获得图片”：向服务端请求本次就诊（医生 id + 患者 id + 日期）的舌苔照，
    // 图片回包暂不解析，先把请求发出去（与“完成”一样经 AppointWidget 打包上包）
    m_getImgBtn = new QPushButton(QStringLiteral("获得图片"), this);
    m_getImgBtn->setMinimumSize(200, 34);
    m_getImgBtn->setStyleSheet(
        "QPushButton{ background:#E6F0FA; color:#1E70BF; border:1px solid #B7D4F2; border-radius:6px; font-size:14px; }"
        "QPushButton:hover{ background:#D6E6F5; }");
    m_getImgBtn->setToolTip(QStringLiteral("向服务端请求该次就诊的舌苔图片（医生 id + 患者 id + 就诊日期）"));
    right->addWidget(m_getImgBtn);
    right->addStretch();

    hMain->addLayout(right, 0);

    connect(m_backBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_doneBtn, &QPushButton::clicked, this, &AppointDetailWidget::onDone);
    connect(m_getImgBtn, &QPushButton::clicked, this, &AppointDetailWidget::onGetTongueImg);
    connect(m_exportBtn, &QPushButton::toggled, this, [this]() { updateExportBtn(); });
    updateExportBtn();

    // “获得图片”的等待超时：服务端在该患者/该日期没有图片时**不回包**（那边直接 return 了），
    // 用个单次定时器兜底提示，别让图片位一直停在“正在获取…”。可重试（按钮一直可用）。
    m_imgTimer = new QTimer(this);
    m_imgTimer->setSingleShot(true);
    m_imgTimer->setInterval(5000);
    connect(m_imgTimer, &QTimer::timeout, this, [this]() {
        if (!m_imgWaiting)
            return;
        m_imgWaiting = false;
        clearTonguePlaceholder(QStringLiteral("未收到图片\n服务端可能没有该次就诊的舌苔图片，可重试"));
    });

    // 本次就诊的图之前已经收过（例如关掉又重开弹窗）就直接显示，不用再请求一次
    showTonguePixmap();
}

void AppointDetailWidget::clearTonguePlaceholder(const QString &tip)
{
    // 注意顺序：QLabel 设了 pixmap 就不再显示文字，所以先清 pixmap 再 setText
    m_tongueLabel->setPixmap(QPixmap());
    m_tongueLabel->setText(tip);
    m_tongueLabel->setToolTip(tip);
}

void AppointDetailWidget::showTonguePixmap()
{
    const QImage &img = CData::tongue_image;
    if (img.isNull() || CData::tongue_image_patient_id != m_rec.patient_id) {
        clearTonguePlaceholder(QStringLiteral("暂无图片\n点下方“获得图片”获取"));
        return;
    }
    // 按图片位大小等比缩放，不拉伸变形（200×250 的框里放原始比例）
    m_tongueLabel->setText(QString());
    m_tongueLabel->setPixmap(QPixmap::fromImage(img).scaled(
        m_tongueLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_tongueLabel->setToolTip(QStringLiteral("舌苔图片 %1×%2（%3）")
                                  .arg(img.width()).arg(img.height())
                                  .arg(CData::tongue_image_file));
    qDebug() << "舌苔图片显示到弹窗: " << img.width() << "x" << img.height()
             << CData::tongue_image_file;
}

void AppointDetailWidget::setTongueImage(const QImage &img, int patientId)
{
    // 认领：只认本次就诊这位患者的图（上一次请求的回包迟到、或服务端串了号都不理）
    if (patientId != m_rec.patient_id) {
        qDebug() << "收到非本次就诊的舌苔图片，忽略: 回包患者" << patientId
                 << " 本次患者" << m_rec.patient_id;
        return;
    }

    m_imgWaiting = false;
    m_imgTimer->stop();

    if (img.isNull()) {   // 收全了但拼图失败
        clearTonguePlaceholder(QStringLiteral("图片不可用\n可重试“获得图片”"));
        return;
    }
    showTonguePixmap();   // 图已在 CData 里（patient_id 刚核对过），直接画
}

void AppointDetailWidget::updateExportBtn()
{
    const bool on = m_exportBtn->isChecked();
    // 文字与配色随开关切换，想取消导出再点一下即可
    m_exportBtn->setText(on ? QStringLiteral("取消") : QStringLiteral("一键导出"));
    m_exportBtn->setToolTip(on ? QStringLiteral("已开启导出：点“完成”时会自动导出诊疗记录 PDF（再点本按钮取消）")
                               : QStringLiteral("开启后，点“完成”时自动把本次诊疗记录导出为 PDF"));
}

QString AppointDetailWidget::recordDate() const
{
    // 服务端下发的 m_rec.time 是原始时间串（形如 "2026-09-12 上午"、"2026/09/12" 等），
    // 这里只把其中的“年月日”抠出来，统一成 yyyy-MM-dd 发给服务端（与 MEDICAL_RECORD_REQ 的日期格式一致）；
    // 抠不到（服务端没给时间/格式不认识）就退回今天，至少保证包里日期非空。
    static const QRegularExpression re(QStringLiteral("(\\d{4})[-/\\.](\\d{1,2})[-/\\.](\\d{1,2})"));
    const QRegularExpressionMatch m = re.match(m_rec.time);
    if (m.hasMatch()) {
        const QDate d(m.captured(1).toInt(), m.captured(2).toInt(), m.captured(3).toInt());
        if (d.isValid())
            return d.toString(QStringLiteral("yyyy-MM-dd"));
    }
    return QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd"));
}

void AppointDetailWidget::onGetTongueImg()
{
    // 请求需要的三项（医生 id / 患者 id / 日期）交给 AppointWidget 打包上包；
    // 回包走 SocketLink::get_tongue_img_success → AppointWidget::flush_tongue_img → 本弹窗 setTongueImage()
    const QString date = recordDate();
    qDebug().noquote() << QStringLiteral("请求舌苔图片 GET_TONGUE_IMG: doctor %1, patient %2, date %3")
                              .arg(m_rec.doctor_id).arg(m_rec.patient_id).arg(date);

    clearTonguePlaceholder(QStringLiteral("正在获取舌苔图片…"));
    m_imgWaiting = true;
    m_imgTimer->start();     // 服务端没这张图时不会回包，靠它兜底提示

    emit to_get_tongue_img(m_rec.doctor_id, m_rec.patient_id, date);
}

void AppointDetailWidget::onDone()
{
    // 警告弹窗二次确认，防止误点“完成”
    int ret = QMessageBox::warning(
        this,
        QStringLiteral("完成确认"),
        QStringLiteral("确认完成本次就诊？\n\n提交后本次预约将标记为已就诊，并保存下方填写的诊断与处方。\n是否继续？"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (ret != QMessageBox::Yes)
        return;

    printAll();

    // 没开“一键导出”就直接关；开了则走异步导出，导出结束后在回调里再 accept()
    if (!exportPdfEnabled()) {
        accept();
        return;
    }
    startExport();
}

void AppointDetailWidget::startExport()
{
    // 导出是异步的（离屏 WebEngine 渲染病历页 + printToPdf），期间禁用按钮，
    // 防止医生连点“完成”或在导出中途关窗导致页面被销毁
    m_exportBtn->setEnabled(false);
    m_backBtn->setEnabled(false);
    m_doneBtn->setEnabled(false);
    m_getImgBtn->setEnabled(false);
    m_doneBtn->setText(QStringLiteral("导出中…"));
    m_exporting = true;

    RecordPdfData rec;
    rec.meet_id      = m_rec.meet_id;
    rec.doctor_id    = m_rec.doctor_id;
    rec.patient_id   = m_rec.patient_id;
    rec.doctor_name  = m_rec.doctor_name;
    rec.patient_name = m_rec.patient_name;
    rec.time         = m_rec.time;
    rec.diagnosis    = diagnosis();
    rec.treat_plan   = treatPlan();
    // 舌苔图片位目前是黑底占位、没设过 pixmap，这里取到的是空图 → 病历页里该区块整块隐藏
    rec.tongue       = m_tongueLabel->pixmap().toImage();

    m_exporter = new RecordPdf(this);   // 挂在弹窗下，随弹窗一起销毁
    connect(m_exporter, &RecordPdf::done, this, [this](const QString &filePath, const QString &err) {
        m_exporting = false;   // 先解除封锁，下面弹提示期间才关得掉
        m_exportedPath = filePath;
        if (filePath.isEmpty()) {
            // 导出失败只警告，不阻断完成就诊（诊断处方照常上包）
            qDebug().noquote() << QStringLiteral("诊疗记录导出失败：") << err;
            QMessageBox::warning(this, QStringLiteral("导出失败"),
                                 QStringLiteral("诊疗记录导出 PDF 失败：\n%1\n\n本次就诊仍可正常完成。")
                                     .arg(err.isEmpty() ? QStringLiteral("未知原因") : err));
        } else {
            qDebug().noquote() << QStringLiteral("诊疗记录已导出：") << filePath;
            QMessageBox::information(this, QStringLiteral("导出成功"),
                                     QStringLiteral("诊疗记录已导出为 PDF：\n%1").arg(filePath));
        }
        accept();
    });
    m_exporter->exportRecord(rec);
}

void AppointDetailWidget::printAll() const
{
    qDebug().noquote() << QStringLiteral("================ 本次就诊信息 ================");
    qDebug().noquote() << QStringLiteral("meet_id     : %1").arg(m_rec.meet_id);
    qDebug().noquote() << QStringLiteral("doctor_id   : %1").arg(m_rec.doctor_id);
    qDebug().noquote() << QStringLiteral("patient_id  : %1").arg(m_rec.patient_id);
    qDebug().noquote() << QStringLiteral("doctor_name : %1").arg(m_rec.doctor_name);
    qDebug().noquote() << QStringLiteral("patient_name: %1").arg(m_rec.patient_name);
    qDebug().noquote() << QStringLiteral("time        : %1").arg(m_rec.time);
    qDebug().noquote() << QStringLiteral("诊断 diagnosis   : %1").arg(m_diagnosisEdit->toPlainText().trimmed());
    qDebug().noquote() << QStringLiteral("处方 prescription: %1").arg(m_prescriptionEdit->toPlainText().trimmed());
    qDebug().noquote() << QStringLiteral("=================================================");
}
