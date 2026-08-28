#include "orderwidget.h"
#include "ui_orderwidget.h"
#include "../MyTcp/protecol.h"
#include <string.h>
#include "../MyTcp/cdata.h"
OrderWidget::OrderWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::OrderWidget)
{
    ui->setupUi(this);
    init_guard_card();
    init_connect();
}

OrderWidget::~OrderWidget()
{
    delete ui;
}

void OrderWidget::init_guard_card()
{
    for(int i=0;i<3;i++){
        for(int j=0;j<7;j++){
            myCards[i][j]=new CustomCard();
            ui->gridLayout->addWidget(myCards[i][j],i,j);
        }
    }

}

void OrderWidget::init_connect()
{
    connect(ui->comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &OrderWidget::on_comboBox_department_currentIndexChanged);
}
void OrderWidget::on_comboBox_department_currentIndexChanged(int index)
{
    QString text = ui->comboBox->currentText();
    qDebug() << "【科室下拉框】当前文本:" << text << "，索引(编号):" << index;
    DOCTOR_INFO_REQ req;
    strncpy_s(req.department, text.toStdString().c_str(), 49);
    req.id=CData::m_id;
    HEAD head;
    head.is_fragment=false;
    head.len=sizeof(req);
    head.type=SERVICE_TYPE::SELECT_DOCTOR;
    QByteArray data;
    int send_size=sizeof(HEAD)+sizeof(req);
    data.resize(send_size);
    // 拷贝 HEAD 和 req 到 data
    memcpy(data.data(), &head, sizeof(HEAD));
    memcpy(data.data() + sizeof(HEAD), &req, sizeof(req));
    emit get_doctor_info(data,send_size);
}

void OrderWidget::on_comboBox_doctor_currentIndexChanged(int index)
{
    QString text = ui->comboBox_2->currentText();
    qDebug() << "【医生下拉框】当前文本:" << text << "，索引(编号):" << index;
}

void OrderWidget::on_comboBox_3_currentIndexChanged(int index)
{
    QString text = ui->comboBox_3->currentText();
    qDebug() << "【第三个下拉框】当前文本:" << text << "，索引(编号):" << index;
}

void OrderWidget::flush_doctor()
{
    // 1. 清空下拉框所有项
    ui->comboBox_2->clear();

    // 2. 添加一个默认提示项（可选，便于用户体验）
    ui->comboBox_2->addItem("请选择医生");

    // 3. 遍历医生信息，将姓名添加到下拉框
    for (const auto& doc : CData::selece_department_info) {
        // 假设 DOCCTOR_INFO 中的 name 为 char[] 类型，使用 fromUtf8 转换
        QString name = QString::fromUtf8(doc.name);
        // 如果 name 是 std::string，则使用 QString::fromStdString(doc.name)
        ui->comboBox_2->addItem(name);
    }

    // 4. 默认选中第一项（"请选择医生"）
    ui->comboBox_2->setCurrentIndex(0);

    // 可选：打印调试信息
    qDebug() << "刷新医生列表，共添加" << CData::selece_department_info.size() << "位医生";
}















