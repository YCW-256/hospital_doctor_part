#include "guardwidget.h"
#include "ui_guardwidget.h"

GuardWidget::GuardWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::GuardWidget)
{
    ui->setupUi(this);
    init_guard_card();
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
