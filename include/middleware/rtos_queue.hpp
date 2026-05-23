#pragma once

#include <cstdint>

#include "FreeRTOS.h"
#include "queue.h"

namespace middleware
{

template <typename T>
class RtosQueue
{
public:
    RtosQueue() = default;

    void create(uint8_t length)
    {
        handle_ = xQueueCreate(length, sizeof(T));
        configASSERT(handle_ != nullptr);
    }

    bool send(const T &item, uint32_t timeoutMs = 0)
    {
        return handle_ != nullptr && xQueueSend(handle_, &item, pdMS_TO_TICKS(timeoutMs)) == pdTRUE;
    }

    bool sendFromIsr(const T &item, BaseType_t *higherPriorityTaskWoken)
    {
        return handle_ != nullptr && xQueueSendFromISR(handle_, &item, higherPriorityTaskWoken) == pdTRUE;
    }

    bool receive(T &item, TickType_t ticksToWait)
    {
        return handle_ != nullptr && xQueueReceive(handle_, &item, ticksToWait) == pdTRUE;
    }

    QueueHandle_t nativeHandle() const
    {
        return handle_;
    }

private:
    QueueHandle_t handle_ = nullptr;
};

} // namespace middleware
