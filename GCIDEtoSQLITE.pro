QT -= gui
QT += sql xml

CONFIG += c++17 console
CONFIG -= app_bundle

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000

SOURCES += \
        main.cpp

HEADERS += \
	data.hpp
