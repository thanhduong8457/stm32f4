#include "FreeRTOS.h"
#include "task.h"

#include "platform_board.hpp"

namespace
{

void writePanicText(const char *text)
{
    auto &uart = platform::selectedBoard().uart();
    uart.send(text);
}

[[noreturn]] void haltAfterPanic()
{
    for (;;)
    {
    }
}

} // namespace

extern "C"
{
    volatile bool g_inPanic = false;
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
    g_inPanic = true;
    taskDISABLE_INTERRUPTS();

    writePanicText("\r\n!!! STACK OVERFLOW in task: ");
    if (pcTaskName != nullptr)
    {
        writePanicText(pcTaskName);
    }
    writePanicText(" !!!\r\n");

    haltAfterPanic();
}

void vApplicationMallocFailedHook(void)
{
    g_inPanic = true;
    taskDISABLE_INTERRUPTS();

    writePanicText("\r\n!!! MALLOC FAILED !!!\r\n");

    haltAfterPanic();
}
