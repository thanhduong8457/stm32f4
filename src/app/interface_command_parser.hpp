#pragma once

#include <cstdint>

#include "app/configuration_manager.hpp"
#include "app/system_messages.hpp"

namespace app
{

enum class InterfaceCommandAction : uint8_t
{
    EnterServiceMode,
    ExitServiceMode,
    ServiceModeAlreadyActive,
    SendToDirector,
    GetParameter,
    SaveConfig,
    LoadConfig,
    Error,
};

struct ParsedInterfaceCommand
{
    InterfaceCommandAction action = InterfaceCommandAction::Error;
    SystemMessage message{SystemCommand::InvalidCommand, MessageSource::Interface, 0};
    ConfigParameter parameter = ConfigParameter::TargetRpm;
    const char *response = "ERR unsupported command\r\n";
    bool validateParameter = false;
    bool reportFailure = false;
};

/** Parse one complete host command without performing I/O or changing state. */
ParsedInterfaceCommand parseInterfaceCommand(const char *line, bool serviceMode);

} // namespace app
