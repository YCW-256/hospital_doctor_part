#ifndef RECORDPDF_H
#define RECORDPDF_H

#include <QImage>
#include <QObject>
#include <QString>

class QWebEngineView;

// 就诊记录（门(急)诊病历）的 PDF 导出工具。
// 调用方：普通医生端“接诊信息”弹窗 AppointDetailWidget —— 打开“一键导出”开关后，点“完成”时导出。
//
// 做法（参照 pans 之外 widget.cpp 里那份内存 HTML 病历页的思路）：
//   用一个**离屏 QWebEngineView** 载入内存 HTML 病历模板（见 .cpp 的 kRecordHtml，
//   版式 = 成都中医药大学附属医院 门(急)诊病历，A4 纸 210mm×297mm），
//   再用模板自带的 JS 桥 fillPatientInfo()/fillClinical() 把数据灌进 DOM，
//   最后 page()->printToPdf() 出 A4 PDF —— **浏览器排版**，HTML/CSS 怎么写 PDF 就怎么出
//   （mm 尺寸、字体、边框、contenteditable 分区都保真），比 QPdfWriter+QTextDocument 那套
//   （只吃 Qt 富文本子集、没有 CSS 盒模型）还原度高得多。
//
// 代价：**全异步**。runJavaScript 与 printToPdf 都不阻塞，本类只在完成时发 done()，
//   不要指望像原来那样同步拿到路径；调用方必须等信号再弹提示/关窗。
// 依赖：hos.pro 第一行已有 webenginewidgets，无需改 QT += 。
struct RecordPdfData {
    int     meet_id    = -1;   // 预约/挂号号（表里作“登记号”）
    int     doctor_id  = -1;   // 接诊医生 id
    int     patient_id = -1;   // 患者 id
    QString doctor_name;       // 接诊医生名（填“医师签名”位）
    QString patient_name;      // 患者名
    QString time;              // 就诊时间（服务端下发的原始时间串，填“日期”位）
    QString diagnosis;         // 诊断
    QString treat_plan;        // 处方 / 治疗方案（填模板里的“治疗意见”）
    QImage  tongue;            // 舌苔图片（可为空：为空则隐藏该区块）
};

class RecordPdf : public QObject
{
    Q_OBJECT

public:
    explicit RecordPdf(QObject *parent = nullptr);
    ~RecordPdf() override;   // 导出没走完就被析构（比如弹窗被强关）时，负责把离屏页面收干净

    // 导出目录：<系统文档目录>/诊疗记录；取不到文档目录则退回用户主目录。不存在时自动创建。
    static QString defaultDir();

    // 文件名主体：“患者名_预约号_日期”（不含扩展名与目录）
    static QString buildFileName(const RecordPdfData &rec);

    // 存到 defaultDir()，文件名 患者名_预约号_yyyyMMdd.pdf，重名自动加序号 _1/_2…
    void exportRecord(const RecordPdfData &rec);

    // 存到调用方指定的路径
    void exportTo(const RecordPdfData &rec, const QString &filePath);

signals:
    // 导出结束（成功/失败都发一次）。成功：filePath 非空、err 空；失败：filePath 空、err 为原因。
    void done(const QString &filePath, const QString &err);

private:
    void finishWith(const QString &filePath, const QString &err);   // 收尾：销毁离屏 view + 发信号

    QWebEngineView *m_view;   // 离屏渲染用（不显示、无父对象，导出完 deleteLater）
    QString         m_path;   // 本次目标路径
};

#endif // RECORDPDF_H
