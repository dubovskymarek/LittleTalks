DEFINES += USE_LITTLE_TALKS

INCLUDEPATH += ../../lib/

SOURCES += \
    ../../lib/LTPlatformAdapter.c \
    ../../lib/LTTopic.c \
    ../../lib/LTDevice.c \
    ../../lib/LittleTalksPrivate.c \
    ../../lib/LittleTalks.c \

HEADERS += \
    ../../lib/LittleTalksSettings.h \
    ../../lib/LTPlatformAdapter.h \
    ../../lib/LTTopic.h \
    ../../lib/LTDevice.h \
    ../../lib/LittleTalksPrivate.h \
    ../../lib/LittleTalks.h \

win32 {
    LIBS += -lwsock32
}
