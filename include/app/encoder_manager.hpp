#pragma once

#include <cstdint>

#include "hal/encoder.hpp"

namespace app
{

class CEO;

class EncoderManager
{
public:
    explicit EncoderManager(hal::IEncoder &encoder);

    void initialize(CEO &director);
    void run();
    int32_t read() const;
    int32_t position() const;

private:
    hal::IEncoder &encoder_;
    CEO *director_ = nullptr;
    int32_t lastPosition_ = 0;
};

} // namespace app
