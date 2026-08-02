#pragma once

#include "platform/stm32f4/board.hpp"

namespace platform
{

using SelectedBoard = stm32f4::Board;

inline SelectedBoard &selectedBoard()
{
    return stm32f4::board();
}

} // namespace platform
