// neschan.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "neschan.h"
#include <iostream>

using namespace std;

inline uint32_t make_argb(uint8_t r, uint8_t g, uint8_t b)
{
    return (r << 16) | (g << 8) | b;
}

#define JOYSTICK_DEADZONE 8000

class neschan_exception : runtime_error 
{
public :
    neschan_exception(const char *msg)
        :runtime_error(string(msg))
    {}
};

class sdl_keyboard_controller : public nes_input_device
{
public:
    sdl_keyboard_controller()
    {
    }

    virtual nes_button_flags poll_status()
    {
        return sdl_keyboard_controller::get_status();
    }

    ~sdl_keyboard_controller()
    {
    }

public:
    static nes_button_flags get_status()
    {
        const Uint8 *states = SDL_GetKeyboardState(NULL);

        uint8_t flags = 0;
        for (int i = 0; i < 8; i++)
        {
            flags <<= 1;
            flags |= states[s_buttons[i]];
        }

        return (nes_button_flags)flags;
    }

private:

    static const SDL_Scancode s_buttons[];
};

const SDL_Scancode sdl_keyboard_controller::s_buttons[] = {
    SDL_SCANCODE_L,
    SDL_SCANCODE_J,
    SDL_SCANCODE_SPACE,
    SDL_SCANCODE_RETURN,
    SDL_SCANCODE_W,
    SDL_SCANCODE_S,
    SDL_SCANCODE_A,
    SDL_SCANCODE_D
};

class sdl_game_controller : public nes_input_device
{
public :
    sdl_game_controller(int id)
        :_id(id)
    {
        _controller = SDL_GameControllerOpen(_id);
        if (_controller == nullptr)
        {
            auto error = SDL_GetError();
            NES_LOG("[NESCHAN_CONTROLLER] Failed to open Game Controller #" << id << ":" << error);
            throw neschan_exception(error);
        }

        NES_LOG("[NESCHAN] Opening GameController #" << std::dec << _id << "..." );
        auto name = SDL_GameControllerName(_controller);
        NES_LOG("    Name = " << name);
        char *mapping = SDL_GameControllerMapping(_controller);
        NES_LOG("    Mapping = " << mapping);

        // Don't need game controller events
        SDL_GameControllerEventState(SDL_DISABLE);
    }

    virtual nes_button_flags poll_status()
    {
        SDL_GameControllerUpdate();

        uint8_t flags = 0;
        for (int i = 0; i < 8; i++)
        {
            flags <<= 1;
            flags |= SDL_GameControllerGetButton(_controller, s_buttons[i]);
        }

        if (_id == 0)
        {
            // Also poll keyboard as well for main player
            flags |= sdl_keyboard_controller::get_status();
        }

        return (nes_button_flags)flags;
    }

    ~sdl_game_controller()
    {
        NES_LOG("[NESCHAN] Closing GameController #" << std::dec << _id << "..." );
        SDL_GameControllerClose(_controller);
        _controller = nullptr;
    }

private :
    int _id;
    SDL_GameController * _controller;

    static const SDL_GameControllerButton s_buttons[];
};

const SDL_GameControllerButton sdl_game_controller::s_buttons[] = {
    SDL_CONTROLLER_BUTTON_A,
    SDL_CONTROLLER_BUTTON_X,
    SDL_CONTROLLER_BUTTON_GUIDE,
    SDL_CONTROLLER_BUTTON_START,
    SDL_CONTROLLER_BUTTON_DPAD_UP,
    SDL_CONTROLLER_BUTTON_DPAD_DOWN,
    SDL_CONTROLLER_BUTTON_DPAD_LEFT,
    SDL_CONTROLLER_BUTTON_DPAD_RIGHT
};

int main(int argc, char *argv[])
{
    // Initialize SDL with everything (video, audio, joystick, events, etc)
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0 )
    {
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR,
            "SDL_Init Error",
            SDL_GetError(),
            NULL);
        return -1;
    }

    const char *error = nullptr;
    if (argc != 2)
    {
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR,
            "Usage error",
            "Usage: neschan <rom_file_path>", 
            NULL);
        return -1;
    }

    SDL_Window *sdl_window;
    SDL_Renderer *sdl_renderer;
    SDL_CreateWindowAndRenderer(PPU_SCREEN_X * 2, PPU_SCREEN_Y * 2, SDL_WINDOW_SHOWN, &sdl_window, &sdl_renderer);

    SDL_SetWindowTitle(sdl_window, "NESChan v0.1 by yizhang82");

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");  // make the scaled rendering look smoother.
    SDL_RenderSetLogicalSize(sdl_renderer, PPU_SCREEN_X, PPU_SCREEN_Y);

    SDL_Texture *sdl_texture = SDL_CreateTexture(
        sdl_renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        PPU_SCREEN_X, PPU_SCREEN_Y);

    INIT_TRACE("neschan.log");

    nes_system system;

    system.power_on();
    
    try
    {
        system.load_rom(argv[1], nes_rom_exec_mode_reset);
    }
    catch (std::exception ex)
    {
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR,
            "ROM load error",
            ex.what(),
            NULL);
        return -1;
    }

    vector<uint32_t> pixels(PPU_SCREEN_Y * PPU_SCREEN_X);

    static uint32_t palette[] = 
    {
        make_argb(84,  84,  84),    make_argb(0,  30, 116),    make_argb(8,  16, 144),    make_argb(48,   0, 136),   make_argb(68,   0, 100),   make_argb(92,   0,  48),   make_argb(84,   4,   0),   make_argb(60,  24,   0),   make_argb(32,  42,   0),   make_argb(8,  58,   0),   make_argb(0,  64,   0),    make_argb(0,  60,   0),    make_argb(0,  50,  60),    make_argb(0,   0,   0),   make_argb(0, 0, 0), make_argb(0, 0, 0),
        make_argb(152, 150, 152),   make_argb(8,  76, 196),    make_argb(48,  50, 236),   make_argb(92,  30, 228),   make_argb(136,  20, 176),  make_argb(160,  20, 100),  make_argb(152,  34,  32),  make_argb(120,  60,   0),  make_argb(84,  90,   0),   make_argb(40, 114,   0),  make_argb(8, 124,   0),    make_argb(0, 118,  40),    make_argb(0, 102, 120),    make_argb(0,   0,   0),   make_argb(0, 0, 0), make_argb(0, 0, 0),
        make_argb(236, 238, 236),   make_argb(76, 154, 236),   make_argb(120, 124, 236),  make_argb(176,  98, 236),  make_argb(228,  84, 236),  make_argb(236,  88, 180),  make_argb(236, 106, 100),  make_argb(212, 136,  32),  make_argb(160, 170,   0),  make_argb(116, 196,   0), make_argb(76, 208,  32),   make_argb(56, 204, 108),   make_argb(56, 180, 204),   make_argb(60,  60,  60),  make_argb(0, 0, 0), make_argb(0, 0, 0),
        make_argb(236, 238, 236),   make_argb(168, 204, 236),  make_argb(188, 188, 236),  make_argb(212, 178, 236),  make_argb(236, 174, 236),  make_argb(236, 174, 212),  make_argb(236, 180, 176),  make_argb(228, 196, 144),  make_argb(204, 210, 120),  make_argb(180, 222, 120), make_argb(168, 226, 144),  make_argb(152, 226, 180),  make_argb(160, 214, 228),  make_argb(160, 162, 160), make_argb(0, 0, 0), make_argb(0, 0, 0)
    };
    assert(sizeof(palette) == sizeof(uint32_t) * 0x40);

    int num_joysticks = SDL_NumJoysticks();
    NES_LOG("[NESCHAN] " << num_joysticks << " JoySticks detected.");
    if (num_joysticks == 0)
    {
        system.input()->register_input(0, std::make_shared<sdl_keyboard_controller>());
    }
    else
    {
        for (int i = 0; i < num_joysticks; i++)
        {
            if (i < NES_MAX_PLAYER)
                system.input()->register_input(i, std::make_shared<sdl_game_controller>(i));
        }
    }

    SDL_Event sdl_event;
    Uint64 prev_counter = SDL_GetPerformanceCounter();
    Uint64 count_per_second = SDL_GetPerformanceFrequency();

    //
    // Game main loop
    //
    bool quit = false;
    while (!quit)
    {
        while (SDL_PollEvent(&sdl_event) != 0)
        {
            switch (sdl_event.type)
            {
                case SDL_QUIT:
                    quit = true;
                    break;
            }
        }

        // 
        // Calculate delta tick as the current frame
        // We ask the NES to step corresponding CPU cycles
        //
        Uint64 cur_counter = SDL_GetPerformanceCounter();

        Uint64 delta_ticks = cur_counter - prev_counter;
        prev_counter = cur_counter;
        if (delta_ticks == 0)
            delta_ticks = 1;
        auto cpu_cycles = ms_to_nes_cycle((double)delta_ticks * 1000 / count_per_second);

        // Avoids a scenario where the loop keeps getting longer
        if (cpu_cycles > nes_cycle_t(NES_CLOCK_HZ))
            cpu_cycles = nes_cycle_t(NES_CLOCK_HZ);

        for (nes_cycle_t i = nes_cycle_t(0); i < cpu_cycles; ++i)
            system.step(nes_cycle_t(1));

        //
        // Copy frame buffer to our texture
        // @TODO - Handle this buffer directly to PPU
        //
        uint32_t *cur_pixel = pixels.data();
        uint8_t *frame_buffer = system.ppu()->frame_buffer();
        for (int y = 0; y < PPU_SCREEN_Y; ++y)
        {
            for (int x = 0; x < PPU_SCREEN_X; ++x)
            {
                *cur_pixel = palette[(*frame_buffer & 0xff)];
                frame_buffer++;
                cur_pixel++;
            }
        }

        //
        // Render
        //
        SDL_UpdateTexture(sdl_texture, NULL, pixels.data(), PPU_SCREEN_X * sizeof(uint32_t));
        SDL_RenderClear(sdl_renderer);
        SDL_RenderCopy(sdl_renderer, sdl_texture, NULL, NULL);
        SDL_RenderPresent(sdl_renderer);
    }

    // make the program return zero
    // unless we catch here
    int errorCode = 0;
    try
    {
        system.save(argv[1]);
    }
    catch (std::exception ex)
    {
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR,
            "SAVE file error",
            ex.what(),
            NULL);
        errorCode = -1;
    }

    // Unregister all inputs and free the game controllers
    system.input()->unregister_all_inputs();

    SDL_DestroyRenderer(sdl_renderer);
    SDL_DestroyTexture(sdl_texture);
    SDL_DestroyWindow(sdl_window);
    // Quit SDL subsystems
    SDL_Quit();

    return errorCode;
}
