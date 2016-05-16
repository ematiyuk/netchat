TEMPLATE = app

TARGET = netchatclient

QT += network widgets multimedia gui

HEADERS += \
    client.h \
    mainwindow.h \
    utils.h \
    constants.h \
    utils.h

SOURCES += \
    client.cpp \
    main.cpp \
    mainwindow.cpp \
    utils.cpp \
    utils.cpp

FORMS += \
    mainwindow.ui

RESOURCES += \
    netchatclient.qrc

win32:RC_ICONS += "data\\icon.ico"
