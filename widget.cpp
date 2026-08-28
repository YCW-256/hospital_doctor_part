#include "widget.h"

#include <QVBoxLayout>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
    //ui->setupUi(this);
    this->setFixedSize(QSize(1000,800));
    // 创建web内核控件
    m_webView = new QWebEngineView(this);

    // 给当前widget设置布局，把webview放进去
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0,0,0,0);
    layout->addWidget(m_webView);
    setLayout(layout);

    // 内存HTML代码
    QString htmlContent = R"HTML(
    <!DOCTYPE html>
    <html lang="zh-CN">
    <head>
        <meta charset="UTF-8">
        <style>
            body{background:#eeeeee;padding:20px;}
            .paper{
                width:210mm;min-height:297mm;background:#ffffff;
                margin:0 auto;padding:25mm;box-sizing: border-box;
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

        <!-- ========== 仅此处允许医生编辑 contenteditable="true" ========== -->
        <div style="margin-top:20px;">
            治疗意见：
            <div id="treat_opinion" contenteditable="true" style="min-height:120px;border:1px #ccc solid;padding:4px;"></div>
        </div>

        <div style="margin-top:40px;text-align:right;" contenteditable="false">
            医师签名：<div id="doctor_sig" contenteditable="true" style="display:inline-block;border:1px #ccc solid;padding:2px;"></div>
            &nbsp;&nbsp;日期：<span id="visit_date"></span>
        </div>
    </div>

    <script>
    //Qt调用这个JS函数填充患者信息（固定区域）
    function fillPatientInfo(name,sex,age,reg,date){
        document.getElementById("span_name").innerText = name;
        document.getElementById("span_sex").innerText = sex;
        document.getElementById("span_age").innerText = age;
        document.getElementById("span_reg").innerText = reg;
        document.getElementById("visit_date").innerText = date;
    }
    //保存时，Qt调用JS获取所有编辑区域内容
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

    // 核心：加载HTML字符串，baseUrl填qrc:/
    m_webView->setHtml(htmlContent, QUrl("qrc:/"));
}

Widget::~Widget()
{
    delete ui;
}
