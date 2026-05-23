#include <cstdio>

#include "FreeRTOS.h"
#include "task.h"

#include "platform_board.hpp"

namespace
{

[[noreturn]] void haltOnFatalError(const char *message)
{
    printf("%s\r\n", message);
    taskDISABLE_INTERRUPTS();
    for (;;)
    {
    }
}

} // namespace

extern "C"
{
    void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName);
    void vApplicationMallocFailedHook(void);
}

int main(void)
{
    auto &board = platform::selectedBoard();
    board.initializeClocks();
    board.application().initialize();
    board.application().startScheduler();
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    printf("[%s] STACK OVERFLOW in task: %s\r\n", __func__, pcTaskName);
    taskDISABLE_INTERRUPTS();
    for (;;)
    {
    }
}

void vApplicationMallocFailedHook(void)
{
    haltOnFatalError("[vApplicationMallocFailedHook] MALLOC FAILED!");
}
