#ifndef RECORDWIDGET_H
#define RECORDWIDGET_H

#include <QString>
#include <QVector>
#include <QWidget>

class QComboBox;
class QDateEdit;
class QHBoxLayout;
class QLabel;
class QVBoxLayout;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTableWidget;
class QTextEdit;

// “查看病例（病历）”页 —— 医生端 MainWindow 左侧导航第 1 项 / 首页“查看病例”图标（toolButton）都指到这里。
// 布局三段：
//   ① 查询条：姓名 + 就诊日期起止 + 查询 / 重置；
//   ② 左侧结果表：就诊日期 | 姓名 | 性别 | 年龄 | 诊断；
//   ③ 右侧详情表单（**可修改**）：就诊日期 / 姓名 / 性别 / 年龄 / 接诊医生 / 诊断 / 治疗意见 + 保存修改 / 撤销修改。
//
// 【本次范围】只做界面 + 按钮信号槽：查询/重置/选中行/保存/撤销全部本地联动（过滤与改写的都是本页的占位数据），
// **数据通信先不做** —— 不走 CData、不组包、不碰 protecol.h / socketlink；
// 将来接服务端时的落点是：把 seed() 的占位数据换成服务端下发、并接上下面两个上行信号（代码里有 TODO 标注）。
struct RecordRow {          // 一行病历（本地数据结构；接入服务端后由 CData 的缓存填充）
    QString visitDate;      // 就诊日期 yyyy-MM-dd
    QString name;           // 患者姓名
    QString sex;            // 性别
    int     age = 0;        // 年龄
    QString doctor;         // 接诊医生
    QString diagnosis;      // 诊断
    QString treatPlan;      // 治疗意见（处方）
};

class RecordWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RecordWidget(QWidget *parent = nullptr);

signals:
    // 预留的上行信号（命名沿用其它 pane 的 to_xxx 习惯）：窗口将来在 init_task_connect 里
    // 接到 SocketLink::send_data 上；**现在还没有接收方**，且不带打包数据 —— 接入时再补 QByteArray 入参。
    void to_query_record();   // TODO: 查询病历（姓名/日期条件待按 protecol.h 组包）
    void to_save_record();    // TODO: 保存修改后的病历

public slots:
    // 预留：服务端回包后由窗口调用，把结果灌进表格（现在只按本地数据重刷）
    void flush_table();

private slots:
    void onQuery();          // 查询：按 姓名 + 就诊日期起止 过滤
    void onReset();          // 重置：清空查询条件并列出全部
    void onSave();           // 保存修改：写回左侧选中行 + 刷新表格（暂不上包）
    void onRevert();         // 撤销修改：把表单还原成选中行的值
    void onRowChanged();     // 左侧选中行变化 → 右侧表单加载该行

private:
    void buildUi();                                  // 组装界面（只建一次）
    void buildQueryBar(QVBoxLayout *root);           // 查询条
    void buildTable(QWidget *parent, QHBoxLayout *row);   // 左侧结果表
    void buildForm(QWidget *parent, QHBoxLayout *row);    // 右侧可编辑详情

    void seed();                                     // 占位数据（CData::is_check，同工作统计页的约定）
    void refreshTable();                             // 按当前条件重刷表格
    bool matchFilter(const RecordRow &r) const;      // 该行是否符合当前查询条件
    int  selectedDataRow() const;                    // 表格当前行 → m_rows 下标（-1 = 未选中）
    void loadRowToForm(int idx);                     // 把某行灌进右侧表单
    void setFormEnabled(bool on);                    // 表单可用性（未选中时应置灰）
    void readFormInto(RecordRow &r) const;           // 把表单内容读回结构体
    bool formDirty() const;                          // 表单有未保存改动（且不是在灌数据、有选中行）
    bool formDiffersFromRow() const;                 // 纯比较：表单 vs 当前选中行（不判 m_loading，供刷新时用）

    // ---- 数据（占位；接入服务端后换成 CData 的缓存）----
    QVector<RecordRow> m_rows;      // 全部病历
    int  m_currentRow;             // 当前选中行在 m_rows 里的下标（-1 = 未选中）
    bool m_loading;                // 正在把行灌进表单（期间不判脏、不弹提示）

    // ---- 控件 ----
    QLineEdit    *m_nameEdit;      // 查询：姓名
    QDateEdit    *m_dateFrom;      // 查询：就诊日期起
    QDateEdit    *m_dateTo;        // 查询：就诊日期止
    QPushButton  *m_queryBtn;      // 查询
    QPushButton  *m_resetBtn;      // 重置
    QTableWidget *m_table;         // 结果表
    QLabel       *m_countLabel;    // “共 N 条”

    QDateEdit   *m_fDate;          // 表单：就诊日期
    QLineEdit   *m_fName;          // 表单：姓名
    QComboBox   *m_fSex;           // 表单：性别
    QSpinBox    *m_fAge;           // 表单：年龄
    QLineEdit   *m_fDoctor;        // 表单：接诊医生
    QTextEdit   *m_fDiagnosis;     // 表单：诊断
    QTextEdit   *m_fTreatPlan;     // 表单：治疗意见（处方）
    QPushButton *m_saveBtn;        // 保存修改
    QPushButton *m_revertBtn;      // 撤销修改
};

#endif // RECORDWIDGET_H
