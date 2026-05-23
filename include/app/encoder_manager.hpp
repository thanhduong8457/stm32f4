#pragma once

#include <cstdint>

#include "hal/encoder.hpp"

namespace app
{

class DirectorManager;

class EncoderManager
{
public:
    explicit EncoderManager(hal::IEncoder &encoder);

    void initialize(DirectorManager &director);
    void run();
    int32_t read() const;
    int32_t position() const;

private:
    hal::IEncoder &encoder_;
    DirectorManager *director_ = nullptr;
    int32_t lastPosition_ = 0;
};

} // namespace app
