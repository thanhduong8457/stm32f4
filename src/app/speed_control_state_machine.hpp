#pragma once

#include <cstdint>

namespace app
{

enum class SpeedControlState : uint8_t
{
    Stopped,
    AwaitingFeedback,
    SoftStart,
    ClosedLoop,
    FeedbackFault,
};

/**
 * Formal control-state owner, kept free of RTOS and hardware dependencies.
 *
 * PidManager supplies target/feedback facts and ramp progress. Keeping the
 * transitions here makes safety behavior explicit and host-testable.
 */
class SpeedControlStateMachine
{
public:
    void updateInputs(bool targetActive, bool feedbackHealthy, bool feedbackSeen);
    void setRampComplete(bool complete);
    void restart(bool targetActive, bool feedbackHealthy, bool feedbackSeen);

    SpeedControlState state() const;
    bool outputEnabled() const;

private:
    SpeedControlState state_ = SpeedControlState::Stopped;
};

} // namespace app
