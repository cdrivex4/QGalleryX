QT += quick core gui qml testlib

TARGET = HelloWorld
TEMPLATE = app
CONFIG += c++17

SOURCES += \
    src/main.cpp \
    tests/tst_helloworld.cpp

RESOURCES += \
    src/main.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH = $$PWD/src

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH = $$PWD/src

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target