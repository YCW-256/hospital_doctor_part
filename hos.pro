QT += core gui widgets network serialport webenginewidgets


CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    MyTcp/cdata.cpp \
    MyTcp/socketlink.cpp \
    Task/businesstask.cpp \
    Task/getdepartmentdoctortask.cpp \
    Task/getguardtask.cpp \
    Task/getinfotask.cpp \
    Task/logintask.cpp \
    Task/tologintask.cpp \
    Tool/myutils.cpp \
    Tool/readutil.cpp \
    main.cpp \
    mainwindow.cpp \
    manngerwindow.cpp \
    pans/appointwidget.cpp \
    pans/childs/chatbom.cpp \
    pans/childs/circularavatar.cpp \
    pans/childs/customcard.cpp \
    pans/childs/mannger/copyschedule.cpp \
    pans/childs/mannger/doctorcard.cpp \
    pans/childs/mannger/smartplan.cpp \
    pans/childs/mannger/smartschedule.cpp \
    pans/childs/medicalcardwidget.cpp \
    pans/childs/selbtn.cpp \
    pans/doctororder.cpp \
    pans/guardwidget.cpp \
    pans/loginwidget.cpp \
    pans/orderwidget.cpp \
    pans/syswidget.cpp \
    widget.cpp

HEADERS += \
    MyTcp/cdata.h \
    MyTcp/protecol.h \
    MyTcp/socketlink.h \
    Task/businesstask.h \
    Task/getdepartmentdoctortask.h \
    Task/getguardtask.h \
    Task/getinfotask.h \
    Task/logintask.h \
    Task/tologintask.h \
    Tool/myutils.h \
    Tool/readutil.h \
    mainwindow.h \
    manngerwindow.h \
    pans/appointwidget.h \
    pans/childs/chatbom.h \
    pans/childs/circularavatar.h \
    pans/childs/customcard.h \
    pans/childs/mannger/copyschedule.h \
    pans/childs/mannger/doctorcard.h \
    pans/childs/mannger/smartplan.h \
    pans/childs/mannger/smartschedule.h \
    pans/childs/medicalcardwidget.h \
    pans/childs/selbtn.h \
    pans/doctororder.h \
    pans/guardwidget.h \
    pans/loginwidget.h \
    pans/orderwidget.h \
    pans/syswidget.h \
    widget.h

FORMS += \
    mainwindow.ui \
    manngerwindow.ui \
    pans/appointwidget.ui \
    pans/childs/chatbom.ui \
    pans/guardwidget.ui \
    pans/loginwidget.ui \
    pans/orderwidget.ui \
    pans/syswidget.ui

RESOURCES += \
    resource.qrc

DESTDIR = $$PWD/bin

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
