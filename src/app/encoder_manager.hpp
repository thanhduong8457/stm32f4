#pragma once

#include <cstdint>

#include "app/system_messages.hpp"
#include "hal/encoder.hpp"
#include "middleware/rtos_queue.hpp"

namespace app
{

class CEO;

class EncoderManager
{
public:
    explicit EncoderManager(hal::IEncoder &encoder);

    void initialize(CEO &director);
    void run();
    bool sendCommand(const EncoderCommand &command, uint32_t timeoutMs = 0);
    int32_t read() const;
    int32_t count() const;
    int32_t rpm() const;
    RotationDirection direction() const;

private:
    void processCommand(const EncoderCommand &command);
    void sample();
    int32_t calculateRpm(int32_t delta) const;

    hal::IEncoder &encoder_;
    CEO *director_ = nullptr;
    middleware::RtosQueue<EncoderCommand> queue_;
    int32_t lastRawCount_ = 0;
    int32_t count_ = 0;
    int32_t rpm_ = 0;
    uint32_t samplePeriodMs_ = 20;
    RotationDirection direction_ = RotationDirection::Stopped;
};

} // namespace app
