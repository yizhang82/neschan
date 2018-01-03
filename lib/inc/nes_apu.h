#pragma once

#include <nes_component.h>

class nes_system;

class nes_audio_device
{
public :
    void read_buffer(uint8_t *buf, size_t buf_size)
    {
        if (_write_offset == _read_offset && !_buf_full)
        {
            memset(buf, 0, buf_size);
            NES_TRACE1("[NES_APU] Not enough buffered audio. " << buf_size << " filled with silence.");
            return;
        }

        _buf_full = false;
        if (_write_offset > _read_offset)
        {
            //  read          write
            //   ^             ^
            // --------------------
            size_t remaining_size = _write_offset - _read_offset;
            size_t copy_size = remaining_size > buf_size ? buf_size : remaining_size;

            memcpy_s(buf, buf_size, _audio_buffer.data() + _read_offset, copy_size);

            // wrap to 0 if _read_offset is at the end
            _read_offset = (_read_offset + copy_size) % _audio_buffer.size();
            if (buf_size > remaining_size)
            {
                buf_size -= remaining_size;
                buf += remaining_size;
                memset(buf, 0, buf_size);
                NES_TRACE1("[NES_APU] Not enough buffered audio. " << buf_size << " filled with silence.");

                return;
            }
        }
        else
        {
            // write         read
            //   ^             ^
            // --------------------
            size_t remaining_size = _audio_buffer.size() - _read_offset;
            size_t first_copy = remaining_size > buf_size ? buf_size : remaining_size;
            memcpy_s(buf, buf_size, _audio_buffer.data() + _read_offset, first_copy);
            buf_size -= first_copy;
            buf += first_copy;
            if (buf_size == 0)
            {
                _read_offset += first_copy;
                return;
            }

            memcpy_s(buf, buf_size, _audio_buffer.data(), _write_offset > buf_size ? buf_size : _write_offset);
            if (buf_size > _write_offset)
            {
                buf_size -= _write_offset;
                buf += _write_offset;
                memset(buf, 0, buf_size);
                NES_TRACE1("[NES_APU] Not enough buffered audio. " << buf_size << " filled with silence.");

                _read_offset = _write_offset;

                return;
            }

            _read_offset += buf_size;
        }
    }

    void write_buffer(const uint8_t *buf, size_t buf_size)
    {
        if (_buf_full)
        {
            NES_TRACE1("[NES_APU] Buffer full - " << buf_size << " bytes lost");
            return;
        }

        if (_write_offset >= _read_offset)
        {
            //  read          write
            //   ^             ^
            // --------------------
            size_t remaining_size = _audio_buffer.size() - _write_offset;
            if (remaining_size >= buf_size)
            {
                memcpy_s(_audio_buffer.data() + _write_offset, remaining_size, buf, buf_size);

                // handle special case remaining_size == buf_size
                _write_offset = (_write_offset + buf_size) % _audio_buffer.size();
                if (_write_offset == _read_offset)
                    _buf_full = true;
            }
            else
            {
                memcpy_s(_audio_buffer.data() + _write_offset, remaining_size, buf, remaining_size);

                buf_size -= remaining_size;
                if (_read_offset < buf_size)
                {
                    NES_TRACE1("[NES_APU] Buffer full - " << buf_size << " bytes lost");
                    return;
                }

                memcpy_s(_audio_buffer.data(), buf_size, buf + remaining_size, buf_size);

                _write_offset = buf_size;
            }
        }
        else
        {
            // write         read
            //   ^             ^
            // --------------------
            size_t remaining_size = _read_offset - _write_offset;
            if (remaining_size < buf_size)
            {
                NES_TRACE1("[NES_APU] Buffer full - " << buf_size << " bytes lost");
                return;
            }

            memcpy_s(_audio_buffer.data() + _write_offset, remaining_size, buf, buf_size);

            _write_offset += buf_size;

            if (_write_offset == _read_offset)
                _buf_full = true;
        }
    }

private :
    bool            _buf_full;              // distingush full or empty when begin == end
    size_t          _write_offset;          // end of data - increment when writing
    size_t          _read_offset;           // begin of data - increment when reading
    vector<uint8_t> _audio_buffer;          // circular audio buffer of desired size
};

class nes_apu_mixer
{
    
};

// 
// Pulse channel that produce square wave
//
class nes_apu_pulse_channel
{
public :
    void init()
    {
        _cur_timer = 0;
        _timer = 0;
    }

    void write_duty(uint8_t val)
    {
        _duty_cycle = (val & 0xc0) >> 6;
        _length_counter_halt = val & 0x20;
        _constant_volume = val & 0x10;
        _volume = val & 0x7;
    }

    void write_sweep(uint8_t val)
    {
        _sweep_enabled = val & 0x80;
        _period = (val & 0x70) >> 4;
        _negate = val & 0x8;
        _shift_count = val & 0x7;
    }

    void write_timer(uint8_t val)
    {
        _timer |= val;
    }

    void write_length_counter(uint8_t val)
    {
        _timer = (_timer & 0xff) | ((val & 0x7) << 8);
        _length_counter = val >> 3;

        // timer gets immediately reset
        _cur_timer = _timer;
    }

private :
    // duty
    uint8_t _duty_cycle;        // which of the duty cycle it is using
    bool _length_counter_halt;  // length counter halt
    bool _constant_volume;      // true if using constant volume, otherwise use volume from envelop
    uint8_t _volume;            // constant volume (if _use_envelop is false), otherwise it is the period of envelop

    // length counter
    uint16_t _cur_timer;
    uint16_t _timer;            // internal waveform generator timer goes from t -> 0 -> t
    uint8_t _length_counter;    // length counter
    uint8_t _divider;

    // sweep
    bool _sweep_enabled;
    uint8_t _period;
    bool _negate;
    uint8_t _shift_count;

private :
    static uint8_t s_duty_cycle[4][8];
};

class nes_apu_triangle_channel
{
};

class nes_apu_noise_channel
{

};

class nes_apu_dmc_channel
{

};

//
// NES APU implementation
// http://wiki.nesdev.com/w/index.php/APU
//
class nes_apu : nes_component

{
public:
    nes_apu();
    ~nes_apu();

public :
    //
    // nes_component overrides
    //
    virtual void power_on(nes_system *system) 
    { 
        _system = system;
        init(); 
    }

    virtual void reset() { init(); }

    virtual void step_to(nes_cycle_t count) {}

private :
    void init()
    {
        // status
        _dmc_enabled = _noise_enabled = _triangle_enabled = _pulse_1_enabled = _pulse_2_enabled = false;

        // frame counter
        _frame_counter_mode = 0;
        _irq_inhibit = true;
    }

public :
    //
    // I/O registers
    //

    void write_status(uint8_t val)
    {
        _dmc_enabled = val & 0x10;
        _noise_enabled = val & 0x8;
        _triangle_enabled = val & 0x4;
        _pulse_2_enabled = val & 0x2;
        _pulse_1_enabled = val & 0x1;
    }

    uint8_t read_status()
    {
        // @TODO
        return 0;
    }

    void write_frame_counter(uint8_t val)
    {
        // @TODO
        _frame_counter_mode = val >> 7;
        _irq_inhibit = val & 0x40;
    }

    void write_pulse_1_duty(uint8_t val)
    {
        
    }

    void write_pulse_1_sweep(uint8_t val)
    {

    }

    void write_pulse_1_timer_low(uint8_t val)
    {

    }

    void write_pulse_1_length_counter(uint8_t val)
    {

    }

private:
    nes_system * _system;

    // status
    bool _dmc_enabled;
    bool _noise_enabled;
    bool _triangle_enabled;
    bool _pulse_2_enabled;
    bool _pulse_1_enabled;

    // frame counter
    uint8_t _frame_counter_mode;
    bool _irq_inhibit;
};
