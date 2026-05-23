#include "app/blink_task.hpp"

#include "FreeRTOS.h"
#include "task.h"

#include "app/app_config.hpp"

namespace app
{

BlinkTask::BlinkTask(hal::IDigitalOutput &led) : led_(led) {}

void BlinkTask::initialize()
{
    led_.initialize();
}

void BlinkTask::run()
{
    for (;;)
    {
        led_.toggle();
        vTaskDelay(pdMS_TO_TICKS(config::kBlinkPeriodMs));
    }
}

} // namespace app
