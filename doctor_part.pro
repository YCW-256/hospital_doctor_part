QT += core gui widgets   network serialport

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    MyTcp/cdata.cpp \
    MyTcp/socketlink.cpp \
    Task/businesstask.cpp \
    Task/logintask.cpp \
    Task/tologintask.cpp \
    main.cpp \
    mainwindow.cpp \
    pans/childs/chatbom.cpp \
    pans/childs/circularavatar.cpp \
    pans/loginwidget.cpp

HEADERS += \
    MyTcp/cdata.h \
    MyTcp/protecol.h \
    MyTcp/socketlink.h \
    Task/businesstask.h \
    Task/logintask.h \
    Task/tologintask.h \
    mainwindow.h \
    pans/childs/chatbom.h \
    pans/childs/circularavatar.h \
    pans/loginwidget.h

FORMS += \
    mainwindow.ui \
    pans/childs/chatbom.ui \
    pans/loginwidget.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DESTDIR = $$PWD/bin

RESOURCES += \
    resource.qrc
