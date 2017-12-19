#pragma once

class nes_ppu : public nes_component
{
public :
    //
    // nes_component overrides
    //
    virtual void power_on(nes_system *system)
    {
        // Do nothing
    }

    virtual void reset()
    {
        // Do nothing
    }

    virtual void step_to(nes_cycle_t count)
    {
        // Do nothing
    }
};