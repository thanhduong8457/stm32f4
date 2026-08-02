#pragma once

#include "app/application.hpp"

namespace platform
{

class IBoard
{
public:
    virtual ~IBoard() = default;
    virtual void initializeClocks() = 0;
    virtual app::Application &application() = 0;
};

} // namespace platform
