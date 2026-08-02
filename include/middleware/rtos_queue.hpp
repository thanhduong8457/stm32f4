#pragma once

#include <cstdint>

#include "FreeRTOS.h"
#include "queue.h"

namespace middleware
{

/**
 * Small type-safe wrapper around a FreeRTOS queue.
 *
 * FreeRTOS queues store raw bytes and expose a C handle. This class keeps the
 * handle private and fixes the item size to sizeof(T), so application code can
 * pass strongly typed messages without repeating queue setup details.
 */
template <typename T>
class RtosQueue
{
public:
    RtosQueue() = default;

    /**
     * Allocate the RTOS queue before any send/receive operation.
     *
     * The queue owns its internal storage in the FreeRTOS heap. A failed
     * allocation trips configASSERT so startup stops immediately instead of
     * continuing with a null queue handle.
     */
    void create(uint8_t length)
    {
        handle_ = xQueueCreate(length, sizeof(T));
        configASSERT(handle_ != nullptr);
    }

    /**
     * Send one item from task context.
     *
     * timeoutMs is converted to ticks here so callers can use millisecond units.
     * A zero timeout makes the call non-blocking.
     */
    bool send(const T &item, uint32_t timeoutMs = 0)
    {
        return handle_ != nullptr && xQueueSend(handle_, &item, pdMS_TO_TICKS(timeoutMs)) == pdTRUE;
    }

    /**
     * Send one item from interrupt context.
     *
     * The caller passes higherPriorityTaskWoken through to FreeRTOS and should
     * call portYIELD_FROM_ISR afterwards when required by the port.
     */
    bool sendFromIsr(const T &item, BaseType_t *higherPriorityTaskWoken)
    {
        return handle_ != nullptr && xQueueSendFromISR(handle_, &item, higherPriorityTaskWoken) == pdTRUE;
    }

    /**
     * Receive one item in task context.
     *
     * ticksToWait is intentionally a TickType_t because most receive sites use
     * FreeRTOS constants such as portMAX_DELAY.
     */
    bool receive(T &item, TickType_t ticksToWait)
    {
        return handle_ != nullptr && xQueueReceive(handle_, &item, ticksToWait) == pdTRUE;
    }

    /**
     * Expose the native handle only for integration points that need FreeRTOS C
     * APIs directly.
     */
    QueueHandle_t nativeHandle() const
    {
        return handle_;
    }

private:
    QueueHandle_t handle_ = nullptr;
};

} // namespace middleware
