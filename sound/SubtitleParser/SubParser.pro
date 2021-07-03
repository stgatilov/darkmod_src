#-------------------------------------------------
#
# Project created by QtCreator 2015-09-26T21:26:31
#
#-------------------------------------------------

QT       -= gui

TARGET = SubParser
TEMPLATE = lib
CONFIG += staticlib

SOURCES += subparser.cpp \
    SubRipParser.cpp \
    SubtitleItem.cpp \
    SubtitleParser.cpp \
    SubtitleParserFactory.cpp \
    SubtitleWord.cpp

HEADERS += subparser.h \
    SubRipParser.h \
    SubtitleItem.h \
    SubtitleParser.h \
    SubtitleParserFactory.h \
    SubtitleWord.h
unix {
    target.path = /usr/lib
    INSTALLS += target
}
