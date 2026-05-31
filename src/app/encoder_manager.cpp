#include "app/encoder_manager.hpp"

#include "FreeRTOS.h"
#include "task.h"

#include "app/app_config.hpp"
#include "app/ceo.hpp"

namespace app
{

EncoderManager::EncoderManager(hal::IEncoder &encoder) : encoder_(encoder) {}

void EncoderManager::initialize(CEO &director)
{
    director_ = &director;
    encoder_.initialize();
}

int32_t EncoderManager::read() const
{
    return encoder_.read();
}

int32_t EncoderManager::position() const
{
    return lastPosition_;
}

void EncoderManager::run()
{
    TickType_t lastWake = xTaskGetTickCount();
    for (;;)
    {
        const int32_t currentPosition = read();
        if (currentPosition != lastPosition_ && director_ != nullptr)
        {
            const SystemMessage message{SystemCommand::EncoderFeedback,
                                        MessageSource::Encoder,
                                        currentPosition};
            (void)director_->sendEvent(message, config::kManagerQueueSendTimeoutMs);
            lastPosition_ = currentPosition;
        }

        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(config::kEncoderSamplePeriodMs));
    }
}

} // namespace app
