#include "app/application.hpp"

#include <cstdio>

#include "FreeRTOS.h"
#include "task.h"

#include "app/app_config.hpp"

namespace app
{
namespace
{

void directorTaskEntry(void *parameters)
{
    static_cast<Application *>(parameters)->director().run();
}

void interfaceTaskEntry(void *parameters)
{
    static_cast<Application *>(parameters)->interface().run();
}

void motorTaskEntry(void *parameters)
{
    static_cast<Application *>(parameters)->motor().run();
}

void encoderTaskEntry(void *parameters)
{
    static_cast<Application *>(parameters)->encoder().run();
}

void blinkTaskEntry(void *parameters)
{
    static_cast<Application *>(parameters)->blink().run();
}

[[noreturn]] void haltOnFatalError(const char *message)
{
    printf("%s\r\n", message);
    taskDISABLE_INTERRUPTS();
    for (;;)
    {
    }
}

} // namespace

Application::Application(DirectorManager &director,
                         InterfaceManager &interface,
                         MotorController &motor,
                         EncoderManager &encoder,
                         BlinkTask &blink)
    : director_(director),
      interface_(interface),
      motor_(motor),
      encoder_(encoder),
      blink_(blink)
{
}

void Application::initialize()
{
    director_.initialize();
    interface_.initialize(director_);
    motor_.initialize();
    encoder_.initialize(director_);
    blink_.initialize();
}

bool Application::createTasks()
{
    return xTaskCreate(directorTaskEntry,
                       "Director",
                       config::kDirectorTaskStackWords,
                       this,
                       config::kDirectorTaskPriority,
                       nullptr) == pdPASS &&
           xTaskCreate(interfaceTaskEntry,
                       "Iface",
                       config::kInterfaceTaskStackWords,
                       this,
                       config::kInterfaceTaskPriority,
                       nullptr) == pdPASS &&
           xTaskCreate(motorTaskEntry,
                       "Motor",
                       config::kMotorTaskStackWords,
                       this,
                       config::kMotorTaskPriority,
                       nullptr) == pdPASS &&
           xTaskCreate(encoderTaskEntry,
                       "Encoder",
                       config::kEncoderTaskStackWords,
                       this,
                       config::kEncoderTaskPriority,
                       nullptr) == pdPASS &&
           xTaskCreate(blinkTaskEntry,
                       "Blink",
                       config::kBlinkTaskStackWords,
                       this,
                       config::kBlinkTaskPriority,
                       nullptr) == pdPASS;
}

[[noreturn]] void Application::startScheduler()
{
    if (!createTasks())
    {
        haltOnFatalError("Failed to create one or more tasks");
    }

    vTaskStartScheduler();
    haltOnFatalError("Scheduler failed to start");
}

DirectorManager &Application::director()
{
    return director_;
}

InterfaceManager &Application::interface()
{
    return interface_;
}

MotorController &Application::motor()
{
    return motor_;
}

EncoderManager &Application::encoder()
{
    return encoder_;
}

BlinkTask &Application::blink()
{
    return blink_;
}

} // namespace app
