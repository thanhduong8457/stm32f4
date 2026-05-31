#include "app/ui_manager.hpp"

#include "FreeRTOS.h"
#include "task.h"

#include "app/app_config.hpp"

namespace app
{

UIManager::UIManager(hal::IDigitalOutput &led) : led_(led)
{
}

void UIManager::initialize()
{
    queue_.create(config::kBlinkEventQueueLength);
    led_.initialize();
    led_.set(false);
}

bool UIManager::sendEvent(BlinkEvent event, uint32_t timeoutMs)
{
    return queue_.send(event, timeoutMs);
}

void UIManager::run()
{
    bool serviceMode = false;
    BlinkEvent event{};

    for (;;)
    {
        if (serviceMode)
        {
            if (queue_.receive(event, portMAX_DELAY))
            {
                handleEvent(event, serviceMode);
            }
            continue;
        }

        if (queue_.receive(event, pdMS_TO_TICKS(config::kBlinkPeriodMs)))
        {
            handleEvent(event, serviceMode);
        }
        else
        {
            led_.toggle();
        }
    }
}

void UIManager::handleEvent(BlinkEvent event, bool &serviceMode)
{
    switch (event)
    {
    case BlinkEvent::ServiceMode:
        serviceMode = true;
        led_.set(false);
        break;

    case BlinkEvent::NormalMode:
        serviceMode = false;
        led_.set(false);
        break;

    case BlinkEvent::SettingSucceeded:
        if (serviceMode)
        {
            blinkStatus(config::kServiceLedSuccessBlinks);
        }
        break;

    case BlinkEvent::SettingFailed:
        if (serviceMode)
        {
            blinkStatus(config::kServiceLedFailureBlinks);
        }
        break;
    }
}

void UIManager::blinkStatus(uint8_t count)
{
    for (uint8_t i = 0; i < count; ++i)
    {
        led_.set(true);
        vTaskDelay(pdMS_TO_TICKS(config::kServiceLedPulseMs));
        led_.set(false);
        vTaskDelay(pdMS_TO_TICKS(config::kServiceLedPulseMs));
    }
}

} // namespace app
