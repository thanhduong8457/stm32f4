#include <cstdio>
#include <cstring>

#include "app/interface_command_parser.hpp"

namespace
{

int failures = 0;

void expect(bool condition, const char *description)
{
    if (!condition)
    {
        std::fprintf(stderr, "FAIL: %s\n", description);
        ++failures;
    }
}

void expectError(const char *line, bool serviceMode, const char *response)
{
    const app::ParsedInterfaceCommand parsed = app::parseInterfaceCommand(line, serviceMode);
    expect(parsed.action == app::InterfaceCommandAction::Error, line);
    expect(std::strcmp(parsed.response, response) == 0, response);
}

} // namespace

int main()
{
    using app::ConfigParameter;
    using app::InterfaceCommandAction;
    using app::SystemCommand;

    auto parsed = app::parseInterfaceCommand("  SERVICE MODE  ", false);
    expect(parsed.action == InterfaceCommandAction::EnterServiceMode,
           "SERVICE MODE enters service mode");

    expectError("STATUS", false, "ERR service mode required\r\n");

    parsed = app::parseInterfaceCommand("SET RPM +1500", true);
    expect(parsed.action == InterfaceCommandAction::SendToDirector, "SET RPM is routed");
    expect(parsed.message.cmd == SystemCommand::SetTargetRpm, "SET RPM command type");
    expect(parsed.message.value == 1500, "SET RPM value");
    expect(parsed.parameter == ConfigParameter::TargetRpm && parsed.validateParameter,
           "SET RPM requests range validation");

    parsed = app::parseInterfaceCommand("SET KP .125", true);
    expect(parsed.message.cmd == SystemCommand::SetKp, "SET KP command type");
    expect(parsed.message.value == 125, "SET KP fixed-point conversion");

    parsed = app::parseInterfaceCommand("SET SAMPLE_TIME 20", true);
    expect(parsed.message.cmd == SystemCommand::SetSampleTime, "SET SAMPLE_TIME command type");
    expect(parsed.message.value == 20, "SET SAMPLE_TIME value");

    parsed = app::parseInterfaceCommand("GET KD", true);
    expect(parsed.action == InterfaceCommandAction::GetParameter, "GET is handled locally");
    expect(parsed.parameter == ConfigParameter::Kd, "GET KD parameter");

    expect(app::parseInterfaceCommand("SAVE CONFIG", true).action ==
               InterfaceCommandAction::SaveConfig,
           "SAVE CONFIG action");
    expect(app::parseInterfaceCommand("LOAD CONFIG", true).action ==
               InterfaceCommandAction::LoadConfig,
           "LOAD CONFIG action");
    expect(app::parseInterfaceCommand("exit", true).action ==
               InterfaceCommandAction::ExitServiceMode,
           "exit action");

    expectError("SET RPM 2147483648", true, "ERR bad RPM value\r\n");
    expectError("SET KP 1.0000", true, "ERR bad KP value\r\n");
    expectError("SET SAMPLE_TIME 20ms", true, "ERR bad SAMPLE_TIME value\r\n");
    expectError("GET SPEED", true, "ERR unsupported GET command\r\n");
    expectError("SET SPEED 10", true, "ERR unsupported SET command\r\n");

    if (failures != 0)
    {
        std::fprintf(stderr, "%d parser test(s) failed\n", failures);
        return 1;
    }

    std::puts("All interface command parser tests passed");
    return 0;
}
