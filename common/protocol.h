#pragma once
#include <QtGlobal>

namespace Protocol {
constexpr quint32 Magic = 0x53534352;
constexpr quint16 Version = 1;
constexpr quint16 HeaderSize = 36;

#pragma pack(push, 1)
struct FrameHeader {
    quint32 magic;
    quint16 version;
    quint16 headerSize;
    quint32 frameSize;
    quint64 frameNumber;
    quint64 timestamp;
    quint16 width;
    quint16 height;
    quint8 format;
    quint8 reserved[3];
};
#pragma pack(pop)
constexpr quint8 FormatJpeg = 1;
constexpr quint8 FormatPng = 2;
} 