QT       += core gui sql serialport
QT += charts
QT += core gui widgets printsupport
QT += core gui widgets charts
QT += core gui sql network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
QT += core gui charts sql network multimedia

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    arduino.cpp \
    badge.cpp \
    connection.cpp \
    cours.cpp \
    eleve.cpp \
    employe.cpp \
    equipement.cpp \
    examen.cpp \
    login.cpp \
    main.cpp \
    mainwindow.cpp \
    vehicule.cpp

HEADERS += \
    arduino.h \
    badge.h \
    connection.h \
    cours.h \
    eleve.h \
    employe.h \
    equipement.h \
    examen.h \
    login.h \
    mainwindow.h \
    vehicule.h

FORMS += \
    login.ui \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    stuff.qrc
