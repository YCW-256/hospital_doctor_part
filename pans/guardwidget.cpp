#include "guardwidget.h"
#include "ui_guardwidget.h"
#include "../Tool/myutils.h"
#include "MyTcp/cdata.h"
QString select_time[3]={"上午","下午","晚上"};

GuardWidget::GuardWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::GuardWidget)
{
    ui->setupUi(this);
    // 标签数组，按顺序对应周一至周日
    dateLabels[0]=ui->label1_1;dateLabels[1]=ui->label2_2;
    dateLabels[2]=ui->label3_3;dateLabels[3]=ui->label4_4;
    dateLabels[4]=ui->label5_5;dateLabels[5]=ui->label6_6;
    dateLabels[6]=ui->label7_7;
    MyUtils::weekAndDate(dateLabels);
    init_guard_card();
    init_connect();
}

GuardWidget::~GuardWidget()
{
    delete ui;
}
// 3 7
void GuardWidget::init_guard_card()
{
    for(int i=0;i<3;i++){
        for(int j=0;j<7;j++){
            myCards[i][j]=new CustomCard();
            myCards[i][j]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            ui->gridLayout->addWidget(myCards[i][j],i,j);
        }
    }


}

void GuardWidget::to_get_guards(){

    HEAD head;
    head.type=SERVICE_TYPE::GET_GUARD;
    GET_GUARD_REQ req;
    char u[10]="内科";
    strcpy(req.department, u);
    req.id=CData::m_id;
    char *day_time=req.start_day;
    QString qday_time = MyUtils::getThisWeekMondayStr();
    // 拷贝，带上结束符'\0'
    QByteArray ba = qday_time.toLatin1();
    memcpy(day_time, ba.constData(), ba.size());
    day_time[ba.size()] = '\0'; // 手动补0终止符

    head.len=sizeof(req);
    QByteArray send_data;
    send_data.resize(512);
    char *p=send_data.data();
    memcpy(p,&head,sizeof(head));
    memcpy(p+sizeof(head),&req,sizeof(req));
    emit send_my_data(send_data,sizeof(head)+sizeof(req));

}

void GuardWidget::flush_table()
{
     qDebug()<<"flush_table";
    if(CData::current_widget!=this){
        return;
    }
    auto& guards = CData::m_get_cards.guards;

    for(int i=0;i<3;i++){
        for(int j=0;j<7;j++){
            auto &item=guards[i][j];
            if(guards[i][j].isfree==true){
                if(myCards[i][j]->getIsfree()==false)myCards[i][j]->switch_color();
                myCards[i][j]->setCardInfo("","","","");
                myCards[i][j]->setFree(true);//A
            }
            else{
                myCards[i][j]->setFree(true);//B   我知道ab出都设成true会是狮山，，但to_deal_leave会取反一次并且在别处引用过，我真的不想重构了
                QString content[4]={item.depart,select_time[item.time],"坐诊",item.name};
                myCards[i][j]->to_deal_leave(content);
            }


        }
    }

}


void GuardWidget::init_connect()
{
    for(int i=0;i<3;i++){
        for(int j=0;j<7;j++){

            connect(myCards[i][j],&CustomCard::clicked,this,[this,i,j](){
                //myCards[i][j]->switch_color();
                qDebug()<<"nothing";
            });


        }
    }

    connect(ui->pushButton,&QPushButton::clicked,this,[this](){
        this->to_get_guards();
    });

}





