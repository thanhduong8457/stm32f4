#include "app/speed_control_state_machine.hpp"

namespace app
{

void SpeedControlStateMachine::updateInputs(bool targetActive, bool feedbackHealthy,
                                            bool feedbackSeen)
{
    if (!targetActive)
    {
        state_ = SpeedControlState::Stopped;
        return;
    }

    if (!feedbackHealthy)
    {
        state_ = feedbackSeen ? SpeedControlState::FeedbackFault
                              : SpeedControlState::AwaitingFeedback;
        return;
    }

    if (!outputEnabled())
    {
        state_ = SpeedControlState::SoftStart;
    }
}

void SpeedControlStateMachine::setRampComplete(bool complete)
{
    if (outputEnabled())
    {
        state_ = complete ? SpeedControlState::ClosedLoop : SpeedControlState::SoftStart;
    }
}

void SpeedControlStateMachine::restart(bool targetActive, bool feedbackHealthy,
                                       bool feedbackSeen)
{
    state_ = SpeedControlState::Stopped;
    updateInputs(targetActive, feedbackHealthy, feedbackSeen);
}

SpeedControlState SpeedControlStateMachine::state() const
{
    return state_;
}

bool SpeedControlStateMachine::outputEnabled() const
{
    return state_ == SpeedControlState::SoftStart || state_ == SpeedControlState::ClosedLoop;
}

} // namespace app
