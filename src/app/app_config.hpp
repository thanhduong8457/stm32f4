#pragma once

#include <cstdint>

#include "FreeRTOS.h"

namespace app::config
{

constexpr uint32_t kBlinkPeriodMs = 1000;
constexpr uint32_t kServiceLedPulseMs = 100;
constexpr uint32_t kDefaultControlLoopPeriodMs = 20;
constexpr uint32_t kMinControlLoopPeriodMs = 5;
constexpr uint32_t kMaxControlLoopPeriodMs = 1000;
constexpr uint32_t kManagerQueueSendTimeoutMs = 10;

constexpr uint16_t kDirectorTaskStackWords = 256;
constexpr uint16_t kInterfaceTaskStackWords = 256;
constexpr uint16_t kMotorTaskStackWords = 192;
constexpr uint16_t kEncoderTaskStackWords = 192;
constexpr uint16_t kUIManagerStackWords = 128;

constexpr UBaseType_t kDirectorTaskPriority = 3;
constexpr UBaseType_t kInterfaceTaskPriority = 3;
constexpr UBaseType_t kMotorTaskPriority = 2;
constexpr UBaseType_t kEncoderTaskPriority = 2;
constexpr UBaseType_t kUIManagerPriority = 1;

constexpr uint8_t kDirectorQueueLength = 8;
constexpr uint8_t kMotorQueueLength = 8;
constexpr uint8_t kEncoderQueueLength = 4;
constexpr uint8_t kBlinkEventQueueLength = 4;
constexpr uint8_t kInterfaceRxQueueLength = 64;
constexpr uint8_t kInterfacePacketMaxLength = 64;

constexpr uint8_t kServiceLedSuccessBlinks = 2;
constexpr uint8_t kServiceLedFailureBlinks = 3;

constexpr int32_t kTargetRpmMin = 0;
constexpr int32_t kTargetRpmMax = 10000;
constexpr int32_t kDefaultTargetRpm = 0;

constexpr int32_t kPidGainScale = 1000;
constexpr int32_t kPidGainMin = 0;
constexpr int32_t kPidGainMax = 100000;
constexpr int32_t kDefaultKp = 1200;
constexpr int32_t kDefaultKi = 80;
constexpr int32_t kDefaultKd = 20;

constexpr int32_t kPwmDutyMinPermille = 0;
constexpr int32_t kPwmDutyMaxPermille = 1000;
constexpr int32_t kSoftStartRampRpmPerSecond = 500;

constexpr int32_t kEncoderCountsPerRevolution = 1024;

} // namespace app::config
