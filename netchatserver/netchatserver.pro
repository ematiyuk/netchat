TEMPLATE = app

TARGET = netchatserver

HEADERS += \
    mainwindow.h \
    client.h \
    constants.h \
    utils.h \
    server.h

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    client.cpp \
    utils.cpp \
    server.cpp

QT += network widgets

FORMS += \
    mainwindow.ui

win32:RC_ICONS += "data\\icon.ico"

RESOURCES += \
    netchatserver.qrc
