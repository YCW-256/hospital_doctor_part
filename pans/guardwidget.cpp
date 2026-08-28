#include "guardwidget.h"
#include "ui_guardwidget.h"
#include "../Tool/myutils.h"
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
            ui->gridLayout->addWidget(myCards[i][j],i,j);
        }
    }

}

void GuardWidget::init_connect()
{
    for(int i=0;i<3;i++){
        for(int j=0;j<7;j++){

            connect(myCards[i][j],&CustomCard::clicked,this,[this,i,j](){
                myCards[i][j]->switch_color();
            });

        }
    }

}





