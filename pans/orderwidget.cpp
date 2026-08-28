#include "orderwidget.h"
#include "ui_orderwidget.h"
#include "../MyTcp/protecol.h"
#include <string.h>
#include "../MyTcp/cdata.h"
#include <QDate>
#include "../Tool/myutils.h"
QString myTime[3]={"上午","下午","晚上"};

OrderWidget::OrderWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::OrderWidget)
{
    ui->setupUi(this);
    // 标签数组，按顺序对应周一至周日
    dateLabels[0]=ui->label1_1;dateLabels[1]=ui->label2_2;
    dateLabels[2]=ui->label3_3;dateLabels[3]=ui->label4_4;
    dateLabels[4]=ui->label5_5;dateLabels[5]=ui->label6_6;
    dateLabels[6]=ui->label7_7;
    qDebug()<<"ss";
    memset(is_repix,0,sizeof(is_repix));



    MyUtils::weekAndDate(dateLabels,current_info);

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
    connect(ui->comboBox_department, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &OrderWidget::on_comboBox_department_currentIndexChanged);

    for(int i=0;i<3;i++){
        for(int j=0;j<7;j++){

            connect(myCards[i][j],&CustomCard::clicked,this,[this,i,j](){
                if(ui->comboBox_department->currentIndex()!=0&&ui->comboBox_doctor->currentIndex()!=0){
                    //标记位，方便传输sql
                    is_repix[i][j]=true;
                    current_info[i][j].id=CData::selece_department_info[ui->comboBox_doctor->currentIndex()-1].id;//这里要减一修正    下选框从1开始，0为占位
                    current_info[i][j].time=i;
                    QString text = ui->comboBox_department->currentText();
                    QByteArray utf8 = text.toUtf8();  // 转为 UTF-8 字节序列
                    strncpy(current_info[i][j].depart, utf8.constData(), sizeof(current_info[i][j].depart) - 1);
                    current_info[i][j].depart[sizeof(current_info[i][j].depart) - 1] = '\0'; // 确保结尾
                    //----------
                    QString content[4];
                    content[0]=ui->comboBox_department->currentText();
                    content[1]=myTime[i];
                    content[2]="坐诊";
                    content[3]=ui->comboBox_doctor->currentText();
                    myCards[i][j]->to_deal_leave(content);
                    current_info[i][j].isfree=myCards[i][j]->getIsfree();


                    qDebug()<<current_info[i][j].date<<"  "<<current_info[i][j].id<<"  "<<current_info[i][j].time<<" "<<current_info[i][j].isfree<<current_info[i][j].depart;
                }
            });
        }
    }

    //保存


}


//信号构造
void OrderWidget::on_comboBox_department_currentIndexChanged(int index)
{
    QString text = ui->comboBox_department->currentText();
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
    QString text = ui->comboBox_doctor->currentText();
    qDebug() << "【医生下拉框】当前文本:" << text << "，索引(编号):" << index;
}

void OrderWidget::flush_doctor()
{
    // 1. 清空下拉框所有项
    ui->comboBox_doctor->clear();

    // 2. 添加一个默认提示项（可选，便于用户体验）
    ui->comboBox_doctor->addItem("请选择医生");

    // 3. 遍历医生信息，将姓名添加到下拉框
    for (const auto& doc : CData::selece_department_info) {
        // 假设 DOCCTOR_INFO 中的 name 为 char[] 类型，使用 fromUtf8 转换
        QString name = QString::fromUtf8(doc.name);
        // 如果 name 是 std::string，则使用 QString::fromStdString(doc.name)
        ui->comboBox_doctor->addItem(name);
    }

    // 4. 默认选中第一项（"请选择医生"）
    ui->comboBox_doctor->setCurrentIndex(0);

    // 可选：打印调试信息
    qDebug() << "刷新医生列表，共添加" << CData::selece_department_info.size() << "位医生";
}















