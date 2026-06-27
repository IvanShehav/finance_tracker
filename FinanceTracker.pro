QT += widgets network

CONFIG += c++17 utf8_source
TEMPLATE = app
TARGET = FinanceTracker

msvc {
    QMAKE_CXXFLAGS += /utf-8
}

SOURCES += \
    src/main.cpp \
    src/Models.cpp \
    src/FinanceManager.cpp \
    src/ChartWidgets.cpp \
    src/InsiderService.cpp \
    src/MarketService.cpp \
    src/MainWindow.cpp

HEADERS += \
    src/Models.h \
    src/FinanceManager.h \
    src/ChartWidgets.h \
    src/InsiderService.h \
    src/MarketService.h \
    src/MainWindow.h
