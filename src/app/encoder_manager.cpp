#include "app/encoder_manager.hpp"

#include "FreeRTOS.h"
#include "task.h"

#include "app/app_config.hpp"
#include "app/ceo.hpp"

namespace app
{

EncoderManager::EncoderManager(hal::IEncoder &encoder) : encoder_(encoder)
{
}

void EncoderManager::initialize(CEO &director)
{
    director_ = &director;
    queue_.create(config::kEncoderQueueLength);
    encoder_.initialize();
    lastRawCount_ = read();
}

bool EncoderManager::sendCommand(const EncoderCommand &command, uint32_t timeoutMs)
{
    return queue_.send(command, timeoutMs);
}

int32_t EncoderManager::read() const
{
    return encoder_.read();
}

int32_t EncoderManager::count() const
{
    return count_;
}

int32_t EncoderManager::rpm() const
{
    return rpm_;
}

RotationDirection EncoderManager::direction() const
{
    return direction_;
}

void EncoderManager::run()
{
    TickType_t lastWake = xTaskGetTickCount();
    EncoderCommand command{};

    for (;;)
    {
        while (queue_.receive(command, 0))
        {
            processCommand(command);
        }

        sample();
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(samplePeriodMs_));
    }
}

void EncoderManager::processCommand(const EncoderCommand &command)
{
    switch (command.type)
    {
    case EncoderCommandType::SetSamplePeriod:
        if (command.value >= static_cast<int32_t>(config::kMinControlLoopPeriodMs) &&
            command.value <= static_cast<int32_t>(config::kMaxControlLoopPeriodMs))
        {
            samplePeriodMs_ = static_cast<uint32_t>(command.value);
        }
        break;
    }
}

void EncoderManager::sample()
{
    const int32_t rawCount = read();
    const int16_t delta = static_cast<int16_t>(rawCount - lastRawCount_);
    lastRawCount_ = rawCount;

    count_ += delta;
    rpm_ = calculateRpm(delta);
    if (delta > 0)
    {
        direction_ = RotationDirection::Cw;
    }
    else if (delta < 0)
    {
        direction_ = RotationDirection::Ccw;
    }
    else
    {
        direction_ = RotationDirection::Stopped;
    }

    if (director_ != nullptr)
    {
        const SystemMessage message{SystemCommand::EncoderFeedback,
                                    MessageSource::Encoder,
                                    count_,
                                    rpm_,
                                    static_cast<int32_t>(direction_),
                                    delta};
        (void)director_->sendEvent(message, 0);
    }
}

int32_t EncoderManager::calculateRpm(int32_t delta) const
{
    if (samplePeriodMs_ == 0U || config::kEncoderCountsPerRevolution <= 0)
    {
        return 0;
    }

    const int64_t signedRpm =
        (static_cast<int64_t>(delta) * 60000LL) /
        (static_cast<int64_t>(config::kEncoderCountsPerRevolution) * samplePeriodMs_);
    return signedRpm < 0 ? static_cast<int32_t>(-signedRpm) : static_cast<int32_t>(signedRpm);
}

} // namespace app
