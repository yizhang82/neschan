#pragma once

#include <cstdint>

#include "nes_cycle.h"

class nes_system;

class nes_component
{
public :
    virtual void power_on(nes_system *system) = 0;

    virtual void reset() = 0;

    virtual void step_to(nes_cycle_t count) = 0;
};
