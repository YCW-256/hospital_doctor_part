#include "recordpdf.h"

#include <QBuffer>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QPageLayout>
#include <QPageSize>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QUrl>
#include <QWebEnginePage>
#include <QWebEngineView>

namespace {

// 病历页模板（内存 HTML，同 widget.cpp 的做法：不落文件，直接 setHtml）。
// 版式 = 成都中医药大学附属医院 门(急)诊病历；纸张 210mm×297mm 就是实尺寸 A4。
// 约定：固定区一律 contenteditable="false"，只有“治疗意见/医师签名”是 contenteditable="true"
//       （本页只用于导出，没人会在这张离屏页面上编辑；保留该属性是为了将来直接当录入页用）。
// 颜色只用黑白：正文/边框一律 #000000，无灰、无底色（灰色只在打印时被 @media print 去掉的屏幕预览里）。
const char *kRecordHtml = R"HTML(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <style>
        /* 屏幕预览：灰底把“纸”衬出来 */
        body{background:#eeeeee;padding:20px;margin:0;}
        .paper{
            width:210mm;min-height:297mm;background:#ffffff;
            margin:0 auto;padding:25mm;box-sizing:border-box;
            font-family:"SimSun","宋体",serif;color:#000000;
        }
        /* 打印（printToPdf 走的就是这套）：去掉灰底与四周留白，纸面即整张 A4 */
        @media print{
            body{background:#ffffff;padding:0;margin:0;}
            .paper{width:auto;min-height:auto;margin:0;padding:25mm;}
        }
    </style>
</head>
<body>
<div class="paper">
    <!-- ========== 固定不可编辑区域 contenteditable="false" ========== -->
    <div contenteditable="false" style="text-align:center;font-size:28px;font-weight:bold;">
        成都中医药大学附属医院
    </div>
    <div contenteditable="false" style="text-align:center;font-size:24px;margin:12px 0 24px;">
        门(急)诊病历
    </div>

    <div contenteditable="false" style="font-size:18px;">
        姓名：<span id="span_name"></span>
        &nbsp;&nbsp;性别：<span id="span_sex"></span>
        &nbsp;&nbsp;年龄：<span id="span_age"></span>
        &nbsp;&nbsp;登记号：<span id="span_reg"></span>
    </div>
    <hr contenteditable="false">

    <!-- 诊断：本次由 Qt 灌入的电子记录，不开放编辑 -->
    <div contenteditable="false" style="margin-top:20px;font-size:18px;">
        诊断：
        <div id="diagnosis" style="min-height:60px;padding:4px;"></div>
    </div>

    <!-- ========== 允许医生编辑 contenteditable="true" ========== -->
    <div style="margin-top:20px;font-size:18px;">
        治疗意见：
        <div id="treat_opinion" contenteditable="true"
             style="min-height:120px;border:1px #000000 solid;padding:4px;"></div>
    </div>

    <!-- 舌苔图像：本次没图时整块隐藏 -->
    <div id="tongue_block" contenteditable="false" style="margin-top:20px;font-size:18px;display:none;">
        舌苔图像：
        <div style="margin-top:6px;">
            <img id="tongue_img" src="" alt="舌苔图像" style="max-width:60mm;border:1px #000000 solid;">
        </div>
    </div>

    <div style="margin-top:40px;text-align:right;font-size:18px;" contenteditable="false">
        医师签名：<div id="doctor_sig" contenteditable="true"
                    style="display:inline-block;min-width:50mm;border:1px #000000 solid;padding:2px;"></div>
        &nbsp;&nbsp;日期：<span id="visit_date"></span>
    </div>
</div>

<script>
// Qt 调用这个 JS 函数填充患者信息（固定区域）
function fillPatientInfo(name,sex,age,reg,date){
    document.getElementById("span_name").innerText = name || "";
    document.getElementById("span_sex").innerText  = sex  || "";
    document.getElementById("span_age").innerText  = age  || "";
    document.getElementById("span_reg").innerText  = reg  || "";
    document.getElementById("visit_date").innerText = date || "";
}
// Qt 调用这个 JS 函数填充诊疗内容（诊断 / 治疗意见 / 医师签名 / 舌苔图 dataURL）
function fillClinical(diagnosis,opinion,signature,tongueDataUrl){
    document.getElementById("diagnosis").innerText = diagnosis || "";
    document.getElementById("treat_opinion").innerText = opinion || "";
    document.getElementById("doctor_sig").innerText = signature || "";
    if (tongueDataUrl){
        document.getElementById("tongue_img").src = tongueDataUrl;
        document.getElementById("tongue_block").style.display = "block";
    }
}
// 保存时，Qt 调用 JS 获取所有编辑区域内容（本页只用于导出，暂未用到；留给以后网页直接录入的场景）
function getEditContent(){
    return {
        treatOpinion: document.getElementById("treat_opinion").innerText,
        doctorSig: document.getElementById("doctor_sig").innerText
    }
}
</script>
</body>
</html>
)HTML";

// 把 QJsonArray 转成 JS 实参列表：["a","b"] -> "a","b"（引号/反斜杠/换行由 JSON 负责转义，避免手工拼串注入）
QString jsArgs(const QJsonArray &arr)
{
    const QByteArray json = QJsonDocument(arr).toJson(QJsonDocument::Compact);
    return QString::fromUtf8(json.mid(1, json.size() - 2));
}

// QImage -> data:image/png;base64,...（空图返回空串）
QString toDataUrl(const QImage &img)
{
    if (img.isNull())
        return QString();
    QByteArray png;
    QBuffer buf(&png);
    buf.open(QIODevice::WriteOnly);
    img.save(&buf, "PNG");
    buf.close();
    return QStringLiteral("data:image/png;base64,") + QString::fromLatin1(png.toBase64());
}

// 组装灌数据的脚本：一行调患者信息，一行调诊疗内容
QString buildFillScript(const RecordPdfData &rec)
{
    QJsonArray patient;
    patient << rec.patient_name
            << QString()                                                   // 性别：目前无数据来源，留空
            << QString()                                                   // 年龄：目前无数据来源，留空
            << (rec.meet_id >= 0 ? QString::number(rec.meet_id) : QString())   // 登记号 = 预约号
            << rec.time;

    QJsonArray clinical;
    clinical << (rec.diagnosis.trimmed().isEmpty() ? QStringLiteral("（未填写）") : rec.diagnosis.trimmed())
             << (rec.treat_plan.trimmed().isEmpty() ? QStringLiteral("（未填写）") : rec.treat_plan.trimmed())
             << rec.doctor_name
             << toDataUrl(rec.tongue);

    return QStringLiteral("fillPatientInfo(%1);fillClinical(%2);")
        .arg(jsArgs(patient), jsArgs(clinical));
}

} // namespace

RecordPdf::RecordPdf(QObject *parent)
    : QObject(parent)
    , m_view(nullptr)
{
}

RecordPdf::~RecordPdf()
{
    // 离屏 view 没有父对象，得自己收；此时若还在打印，QtWebEngine 会自己中止，
    // 而 printToPdf/runJavaScript 的回调因为接收者是 this（已被析构）不会再来，安全。
    if (m_view) {
        m_view->deleteLater();
        m_view = nullptr;
    }
}

QString RecordPdf::defaultDir()
{
    QString base = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (base.isEmpty())
        base = QDir::homePath();
    QString dir = base + QStringLiteral("/诊疗记录");
    QDir().mkpath(dir);   // 已存在返回 false，不影响
    return dir;
}

QString RecordPdf::buildFileName(const RecordPdfData &rec)
{
    // 患者名做文件名安全处理（去掉 Windows 非法字符），空名退回“患者”
    QString name = rec.patient_name.trimmed();
    name.remove(QRegularExpression(QStringLiteral("[\\\\/:*?\"<>|]")));
    if (name.isEmpty())
        name = QStringLiteral("患者");

    const QString meet = rec.meet_id >= 0 ? QString::number(rec.meet_id) : QStringLiteral("无号");
    return QStringLiteral("%1_%2_%3")
        .arg(name, meet, QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd")));
}

void RecordPdf::exportRecord(const RecordPdfData &rec)
{
    const QString dir = defaultDir();
    const QString baseName = buildFileName(rec);
    QString path = QStringLiteral("%1/%2.pdf").arg(dir, baseName);
    for (int n = 1; QFile::exists(path); ++n)   // 同名（同患者同日多次导出）自动加序号
        path = QStringLiteral("%1/%2_%3.pdf").arg(dir, baseName).arg(n);
    exportTo(rec, path);
}

void RecordPdf::exportTo(const RecordPdfData &rec, const QString &filePath)
{
    m_path = filePath;

    if (m_view) {          // 同一个 RecordPdf 被复用导出第二次时，先清掉上一次的离屏页面
        m_view->deleteLater();
        m_view = nullptr;
    }

    // 离屏页面：不显示、不挂父对象（只管渲染，渲染完自己 deleteLater）
    m_view = new QWebEngineView;
    m_view->resize(794, 1123);   // A4@96dpi，仅作排版参考；真正分页由 printToPdf 的 QPageLayout 决定

    connect(m_view, &QWebEngineView::loadFinished, this, [this, rec](bool ok) {
        if (!ok) {
            finishWith(QString(), QStringLiteral("病历页面加载失败（QtWebEngine 未就绪？）"));
            return;
        }
        // runJavaScript 也是异步的：回调里再打印，确保 DOM 已经灌完
        m_view->page()->runJavaScript(buildFillScript(rec), [this](const QVariant &) {
            // 0 边距：留白交给页面里 .paper 的 25mm padding，避免与引擎边距叠加
            const QPageLayout layout(QPageSize(QPageSize::A4), QPageLayout::Portrait, QMarginsF(0, 0, 0, 0));
            m_view->page()->printToPdf(m_path, layout);
        });
    });

    // printToPdf 只能靠这个信号知道结果
    connect(m_view->page(), &QWebEnginePage::pdfPrintingFinished, this,
            [this](const QString &filePath, bool success) {
        if (success && QFile::exists(filePath))
            finishWith(filePath, QString());
        else
            finishWith(QString(), QStringLiteral("PDF 写入失败（目标目录不可写？）：%1").arg(filePath));
    });

    // baseUrl 给 qrc:/ （页面里以后要引 qrc 图片时相对路径才解析得到）
    m_view->setHtml(QString::fromUtf8(kRecordHtml), QUrl(QStringLiteral("qrc:/")));
}

void RecordPdf::finishWith(const QString &filePath, const QString &err)
{
    if (m_view) {
        m_view->deleteLater();
        m_view = nullptr;
    }
    emit done(filePath, err);
}
