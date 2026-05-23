#pragma once

#include <cstdint>

#include "FreeRTOS.h"

namespace app::config
{

constexpr uint32_t kBlinkPeriodMs = 1000;
constexpr uint32_t kEncoderSamplePeriodMs = 20;
constexpr uint32_t kManagerQueueSendTimeoutMs = 10;

constexpr uint16_t kDirectorTaskStackWords = 256;
constexpr uint16_t kInterfaceTaskStackWords = 256;
constexpr uint16_t kMotorTaskStackWords = 192;
constexpr uint16_t kEncoderTaskStackWords = 192;
constexpr uint16_t kBlinkTaskStackWords = 128;

constexpr UBaseType_t kDirectorTaskPriority = 3;
constexpr UBaseType_t kInterfaceTaskPriority = 3;
constexpr UBaseType_t kMotorTaskPriority = 2;
constexpr UBaseType_t kEncoderTaskPriority = 2;
constexpr UBaseType_t kBlinkTaskPriority = 1;

constexpr uint8_t kDirectorQueueLength = 8;
constexpr uint8_t kMotorQueueLength = 4;
constexpr uint8_t kInterfaceRxQueueLength = 64;
constexpr uint8_t kInterfacePacketMaxLength = 32;

constexpr int32_t kMotorTargetMin = 0;
constexpr int32_t kMotorTargetMax = 180;

} // namespace app::config
