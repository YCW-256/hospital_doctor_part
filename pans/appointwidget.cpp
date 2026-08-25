#include "appointwidget.h"
#include "ui_appointwidget.h"

AppointWidget::AppointWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AppointWidget)
{
    ui->setupUi(this);
}

AppointWidget::~AppointWidget()
{
    delete ui;
}
