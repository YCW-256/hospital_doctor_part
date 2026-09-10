#ifndef RECORDWIDGET_H
#define RECORDWIDGET_H

#include <QByteArray>
#include <QHash>
#include <QString>
#include <QVector>
#include <QWidget>

class QComboBox;
class QDateEdit;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTableWidget;
class QTextEdit;
class QVBoxLayout;

// “查看病例（病历）”页 —— 医生端 MainWindow 左侧导航第 1 项 / 首页“查看病例”图标（toolButton）都指到这里。
//
// 流程照服务端的两套协议走（见 protecol.h）：
//   ① 查询 → 第一套 GET_MEDICAL_RECORD  → 列表：就诊日期 | 姓名 | 患者编号 | 主要症状 | 状态
//   ② 选中某行 → 第二套 GET_MEDICAL_RECORD_DETAIL（带 record_id）→ 详情：性别/年龄/接诊医生/诊断/治疗意见全文
// 详情没到之前，右侧只显示列表已有的字段，详情字段置灰 + 提示“详情加载中”。
//
// 【当前状态】两套**都已打通**：发送侧 组包 + 信号 + 窗口接到 SocketLink::send_data；
// 回包侧 recv_data 分支 → GetMedicalRecordTask/GetMedicalRecordDetailTask 写 CData →
// *_success 信号 → 窗口调 flush_table()/flush_detail() 从 CData 灌本页。
// 本页**不再造占位数据**（原 seed() 已删），列表/详情一律来自服务端。
struct RecordRow {              // 第一套（列表）信息
    int     recordId = -1;      // 病历编号（第二套请求带它）
    int     patientId = -1;     // 患者编号
    int     state = 0;          // 0 正常 1 作废
    QString visitDate;          // 病历记录时间 yyyy-MM-dd HH:mm:ss（列表里按日期展示）
    QString name;               // 患者姓名
    QString mainSymptom;        // 主要症状摘要
};

struct RecordDetail {           // 第二套（详情）信息
    int     recordId = -1;
    int     patientId = -1;
    int     state = 0;
    QString name;               // 患者姓名（只读展示）
    QString sex;                // 性别（只读展示）
    int     age = 0;            // 年龄（只读展示）
    QString visitDate;          // 病历记录时间（可改）
    QString doctor;             // 接诊医生姓名（只读展示）
    QString mainSymptom;        // 主要症状（可改）
    QString diagnosis;          // 诊断（可改）
    QString treatPlan;          // 治疗意见 / 处方（可改）
};

class RecordWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RecordWidget(QWidget *parent = nullptr);

signals:
    // 上行信号（沿用其它 pane 的 to_xxx 习惯）：窗口在 init_task_connect 里接到 SocketLink::send_data。
    // data 已按 protecol.h 组包（HEAD + REQ），len 是整包字节数。
    void to_query_record(const QByteArray data, int len);    // 第一套：按医生 id 查列表
    void to_record_detail(const QByteArray data, int len);   // 第二套：按 record_id 查详情
    // 保存修改：**复用新增病例那套包** —— HEAD(type=DOCTOR_SET_RECORD) + SET_RECORD_REQ，
    // 和 appointwidget::sendRecord() 逐字段一致（只把 meet_id 填 0，服务端用不上它）。
    void to_save_record(const QByteArray data, int len);

public slots:
    // 下行槽：窗口接到 SocketLink::get_medical_record_success / *_detail_success 后调用
    void flush_table();    // 把 CData::medical_record_list 灌进列表并刷新界面
    void flush_detail();   // 把 CData::medical_record_details 里新到的详情灌进表单

private slots:
    void onQuery();          // 查询：发第一套请求（+ 占位数据阶段先按本地过滤刷列表）
    void onReset();          // 重置：清空查询条件并列出全部
    void onSave();           // 保存修改：写回当前记录的详情（暂不上包）
    void onRevert();         // 撤销修改：把表单还原成当前记录的详情
    void onRowChanged();     // 选中行变化 → 发第二套请求 + 先用列表字段填表

private:
    void buildUi();                                       // 组装界面（只建一次）
    void buildQueryBar(QVBoxLayout *root);                // 查询条
    void buildTable(QWidget *parent, QHBoxLayout *row);    // 左侧列表
    void buildForm(QWidget *parent, QHBoxLayout *row);     // 右侧详情表单

    QByteArray makeQueryPack(int *outSize) const;                 // 第一套组包 HEAD + MEDICAL_RECORD_REQ
    QByteArray makeDetailPack(int recordId, int *outSize) const;  // 第二套组包 HEAD + MEDICAL_RECORD_DETAIL_REQ
    QByteArray makeSavePack(const RecordDetail &d, int *outSize) const;  // 保存组包 HEAD + SET_RECORD_REQ（同新增病例）

    void refreshTable();                             // 按当前条件重刷表格
    bool matchFilter(const RecordRow &r) const;      // 该行是否符合当前查询条件
    int  selectedDataRow() const;                    // 表格当前行 → m_rows 下标（-1 = 未选中）
    const RecordDetail *currentDetail() const;       // 当前选中行的详情（没有则 nullptr）
    RecordDetail detailFromRow(const RecordRow &r) const;  // 用列表字段造一条“详情壳”（诊断/处方留空）
    void requestDetailIfNeeded();                    // 当前选中行的详情没缓存就发第二套请求
    void fillFormFromRow(const RecordRow &r);        // 用列表字段填表（详情字段清空）
    void fillFormFromDetail(const RecordDetail &d);  // 用详情填表
    void setDetailEnabled(bool on);                  // 详情字段/按钮可用性（详情没到时置灰）
    void readFormInto(RecordDetail &d) const;        // 把表单可改字段读回结构体
    bool formDirty() const;                          // 表单有未保存改动（且不是在灌数据、有选中行）
    bool formDiffersFromDetail() const;              // 纯比较：表单 vs 当前记录详情（不判 m_loading）
    bool formDiffersFrom(const RecordDetail &base) const;   // 纯比较：表单 vs 任意一份详情

    // ---- 数据（列表/详情都来自 CData，见 flush_table()/flush_detail()）----
    QVector<RecordRow>       m_rows;      // 列表（CData::medical_record_list 的界面版）
    QHash<int, RecordDetail> m_details;   // 详情缓存，key = recordId（CData::medical_record_details 的界面版）
    int  m_currentRow;                    // 当前选中行在 m_rows 里的下标（-1 = 未选中）
    bool m_loading;                       // 正在把数据灌进表单（期间不判脏、不弹提示）
    int  m_serverTotal;                   // 服务端报的符合条件总条数（-1 = 当前列表不是服务端直出的，别显示）
    int  m_pendingDetailId;               // 正在等第二套回包的 record_id（-1 = 没在等）
    RecordDetail m_pendingBase;           // 发详情请求那一刻表单里的内容（判断等回包期间医生有没有动过手）

    // ---- 控件 ----
    QLineEdit    *m_nameEdit;      // 查询：姓名
    QDateEdit    *m_dateFrom;      // 查询：就诊日期起
    QDateEdit    *m_dateTo;        // 查询：就诊日期止
    QPushButton  *m_queryBtn;      // 查询
    QPushButton  *m_resetBtn;      // 重置
    QTableWidget *m_table;         // 列表
    QLabel       *m_countLabel;    // “共 N 条”

    QLabel      *m_formTip;        // 表单右上角提示（详情加载中 / 已加载）
    QDateEdit   *m_fDate;          // 表单：就诊日期（可改）
    QLineEdit   *m_fName;          // 表单：姓名（只读）
    QLineEdit   *m_fPatientId;     // 表单：患者编号（只读）
    QComboBox   *m_fSex;           // 表单：性别（只读）
    QSpinBox    *m_fAge;           // 表单：年龄（只读）
    QLineEdit   *m_fDoctor;        // 表单：接诊医生（只读）
    QTextEdit   *m_fSymptom;       // 表单：主要症状（可改）
    QTextEdit   *m_fDiagnosis;     // 表单：诊断（可改）
    QTextEdit   *m_fTreatPlan;     // 表单：治疗意见（可改）
    QPushButton *m_saveBtn;        // 保存修改
    QPushButton *m_revertBtn;      // 撤销修改
};

#endif // RECORDWIDGET_H
