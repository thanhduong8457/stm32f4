#include <cstdio>

#include "app/speed_control_state_machine.hpp"

namespace
{

int failures = 0;

void expectState(const app::SpeedControlStateMachine &machine, app::SpeedControlState expected,
                 const char *description)
{
    if (machine.state() != expected)
    {
        std::fprintf(stderr, "FAIL: %s\n", description);
        ++failures;
    }
}

} // namespace

int main()
{
    using app::SpeedControlState;
    app::SpeedControlStateMachine machine;

    expectState(machine, SpeedControlState::Stopped, "controller starts stopped");

    machine.updateInputs(true, false, false);
    expectState(machine, SpeedControlState::AwaitingFeedback,
                "target waits for the first encoder sample");
    machine.setRampComplete(true);
    expectState(machine, SpeedControlState::AwaitingFeedback,
                "ramp completion cannot bypass the feedback gate");

    machine.updateInputs(true, true, true);
    expectState(machine, SpeedControlState::SoftStart,
                "fresh feedback starts the soft-start ramp");

    machine.setRampComplete(true);
    expectState(machine, SpeedControlState::ClosedLoop,
                "completed ramp enters closed-loop control");

    machine.updateInputs(true, false, true);
    expectState(machine, SpeedControlState::FeedbackFault,
                "lost feedback enters a visible fault state");

    machine.updateInputs(true, true, true);
    expectState(machine, SpeedControlState::SoftStart,
                "feedback recovery restarts through soft-start");

    machine.updateInputs(false, true, true);
    expectState(machine, SpeedControlState::Stopped, "clearing the target stops control");

    machine.restart(true, true, true);
    expectState(machine, SpeedControlState::SoftStart,
                "PID reset with a target restarts soft-start");

    if (failures != 0)
    {
        std::fprintf(stderr, "%d speed-control state test(s) failed\n", failures);
        return 1;
    }

    std::puts("All speed-control state-machine tests passed");
    return 0;
}
