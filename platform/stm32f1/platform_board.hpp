#pragma once

#include "platform/stm32f1/board.hpp"

namespace platform
{

using SelectedBoard = stm32f1::Board;

inline SelectedBoard &selectedBoard()
{
    return stm32f1::board();
}

} // namespace platform
