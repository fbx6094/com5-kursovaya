#pragma once
#include <QtGlobal>

namespace Protocol {
constexpr quint16 tcpPort = 5000;
constexpr quint16 udpPort = 5001;

inline constexpr char discoveryRequest[] = "DISCOVER_SCREEN_SERVER";
inline constexpr char discoveryResponce[] = "Screen_server";

constexpr quint32 maxframeSize = 20 * 1024 * 1024;
constexpr quint64 maxQueueBytes = 350000;

using packetHeader = quint32;
} 