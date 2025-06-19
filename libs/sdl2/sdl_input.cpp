#pragma once

#include "../input/input_state.hpp"
#include "../util/numeric.hpp"
#include "sdl_include.hpp"

namespace num = numeric;


/* add/remove inputs */

namespace sdl
{
    static inline u64 to_input_handle(SDL_JoystickID id)
    {
        return (u64)(id + 1024);
    }

    
    static void add_gamepad_input(input::InputArray& inputs, SDL_JoystickID id)
    {
        auto handle = to_input_handle(id);
        for (u32 i = 0; i < input::MAX_GAMEPADS; i++)
        {
            if (inputs.prev().gamepads[i].handle)
            {
                continue;
            }

            inputs.prev().gamepads[i].handle = handle;
            inputs.curr().gamepads[i].handle = handle;
            inputs.n_gamepads++;

        #ifdef PRINT_MESSAGES
            printf("Gamepad id %d added\n", id);
        #endif

            break;
        }
    }


    static void add_joystick_input(input::InputArray& inputs, SDL_JoystickID id)
    {
        auto handle = to_input_handle(id);
        for (u32 i = 0; i < input::MAX_JOYSTICKS; i++)
        {
            if (inputs.prev().joysticks[i].handle)
            {
                continue;
            }

            inputs.prev().joysticks[i].handle = handle;
            inputs.curr().joysticks[i].handle = handle;
            inputs.n_joysticks++;

        #ifdef PRINT_MESSAGES
            printf("Joystick id %d added\n", id);
        #endif

            break;
        }
    }


    static void remove_gamepad_input(input::InputArray& inputs, SDL_JoystickID id)
    {
        auto handle = to_input_handle(id);
        for (u32 i = 0; i < input::MAX_GAMEPADS; i++)
        {
            if (inputs.curr().gamepads[i].handle == handle)
            {
                inputs.prev().gamepads[i].handle = 0;
                inputs.curr().gamepads[i].handle = 0;
                inputs.n_gamepads--;

            #ifdef PRINT_MESSAGES
                printf("Gamepad id %d removed\n", id);
            #endif

                break;
            }
        }
    }


    static void remove_joystick_input(input::InputArray& inputs, SDL_JoystickID id)
    {
        auto handle = to_input_handle(id);
        for (u32 i = 0; i < input::MAX_JOYSTICKS; i++)
        {
            if (inputs.curr().joysticks[i].handle == handle)
            {
                inputs.prev().joysticks[i].handle = 0;
                inputs.curr().joysticks[i].handle = 0;
                inputs.n_joysticks--;

            #ifdef PRINT_MESSAGES
                printf("Joystick id %d removed\n", id);
            #endif

                break;
            }
        }
    }


    static i32 find_gamepad(SDL_JoystickID id, input::Input const& input)
    {
        auto handle = to_input_handle(id);

        for (i32 i = 0; i < input::MAX_GAMEPADS; i++)
        {
            if (handle == input.gamepads[i].handle)
            {
                return i;
            }
        }

        return -1;
    }


    static i32 find_joystick(SDL_JoystickID id, input::Input const& input)
    {
        auto handle = to_input_handle(id);

        for (i32 i = 0; i < input::MAX_JOYSTICKS; i++)
        {
            if (handle == input.joysticks[i].handle)
            {
                return i;
            }
        }

        return -1;
    }
}


/* devices */

namespace sdl
{
    template <class T, u32 CAP>
    class DeviceArray
    {
    public:
        static constexpr u32 capacity = CAP;

        T data[CAP];
    };


    class GamepadDevice
    {
    public:
        SDL_JoystickID id = -1;
        SDL_GameController* gamepad = 0;
        SDL_Haptic* rumble = 0;
    };


    class JoystickDevice
    {
    public:
        SDL_JoystickID id = -1;
        SDL_Joystick* joystick = 0;
    };


    using GamepadDeviceArray = DeviceArray<GamepadDevice, input::MAX_GAMEPADS>;
    using JoystickDeviceArray = DeviceArray<JoystickDevice, input::MAX_JOYSTICKS>;


    class InputDeviceList
    {
    public:
        GamepadDeviceArray gamepads;
        JoystickDeviceArray joysticks;
    };

}


/* close devices */

namespace sdl
{
    static void close_device(GamepadDevice& device)
    {
        if (device.gamepad)
        {
            SDL_GameControllerClose(device.gamepad);
            device.gamepad = 0;
        }

        if (device.rumble)
        {
            SDL_HapticClose(device.rumble);
            device.rumble = 0;
        }

        device.id = -1;
    }


    static void close_device(JoystickDevice& device)
    {
        if (device.joystick)
        {
            SDL_JoystickClose(device.joystick);
            device.joystick = 0;
            device.id = -1;
        }
    }


    static void close_input_devices(InputDeviceList& devices)
    {
        for (u32 i = 0; i < devices.gamepads.capacity; i++)
        {
            close_device(devices.gamepads.data[i]);
        }
        
        for (u32 i = 0; i < devices.joysticks.capacity; i++)
        {
            close_device(devices.joysticks.data[i]);
        }
    }


    static void remove_input_device(InputDeviceList& devices, SDL_JoystickID id, input::InputArray& inputs)
    {
        for (u32 i = 0; i < input::MAX_GAMEPADS; i++)
        {
            auto& d_gamepad = devices.gamepads.data[i];
            if (id == d_gamepad.id)
            {
                close_device(d_gamepad);
                remove_gamepad_input(inputs, id);
                return;
            }
        }

        for (u32 i = 0; i < input::MAX_JOYSTICKS; i++)
        {
            auto& d_joystick = devices.joysticks.data[i];
            if (id == d_joystick.id)
            {
                close_device(d_joystick);
                remove_joystick_input(inputs, id);
                return;
            }
        }
    }
}


/* open devices */

namespace sdl
{
    static SDL_JoystickID add_gamepad_device(GamepadDeviceArray& devices, int index)
    {
        SDL_JoystickID id = -1;

    #ifndef NO_GAMEPAD

        for (u32 i = 0; i < devices.capacity; i++)
        {
            auto& device = devices.data[i];
            if (device.gamepad)
            {
                continue;
            }

            auto gc = SDL_GameControllerOpen(index);
            if (!gc)
            {
                sdl::print_error("SDL_GameControllerOpen()");
                return -1;
            }

            auto js = SDL_GameControllerGetJoystick(gc);
            if (!js)
            {
                SDL_GameControllerClose(gc);
                sdl::print_error("SDL_GameControllerGetJoystick()");
                return -1;
            }

            auto h = SDL_HapticOpenFromJoystick(js);
            if (!h)
            {
                sdl::print_message("no rumble from joystick");
            }
            else if (SDL_HapticRumbleInit(h) != 0)
            {
                sdl::print_error("SDL_HapticRumbleInit()");
                SDL_HapticClose(h);
            }
            else
            {
                sdl::print_message("found a rumble");
            }

            id = SDL_JoystickInstanceID(js);
            if (id < 0)
            {
                sdl::print_error("SDL_JoystickInstanceID()");
                return id;
            }

            device.id = id;
            device.gamepad = gc;
            device.rumble = h;
            break;
        }

    #endif

        return id;
    }


    static SDL_JoystickID add_joystick_device(JoystickDeviceArray& devices, int index)
    {
        SDL_JoystickID id = -1;

    #ifndef NO_JOYSTICK
    
        for (u32 i = 0; i < devices.capacity; i++)
        {
            auto& device = devices.data[i];
            if (device.joystick)
            {
                continue;
            }

            auto js = SDL_JoystickOpen(index);
            if (!js)
            {
                sdl::print_error("SDL_JoystickOpen()");
                return -1;
            }
            
            id = SDL_JoystickInstanceID(js);
            if (id < 0)
            {
                sdl::print_error("SDL_JoystickInstanceID()");
                return id;
            }

            device.id = id;
            device.joystick = js;
            break;
        }

        //auto name = SDL_JoystickName(js);
        //auto axes = SDL_JoystickNumAxes(js);
        //auto buttons = SDL_JoystickNumButtons(js);
        //printf("%s | %d | %d\n", name, axes, buttons);

    #endif

        return id;
    }


    static void add_input_device(InputDeviceList& devices, SDL_JoystickID id, input::InputArray& inputs)
    {
        auto handle = to_input_handle(id);

        if (SDL_IsGameController(id))
        {
            if (find_gamepad(id, inputs.curr()) >= 0)
            {
                return;
            }

            id = add_gamepad_device(devices.gamepads, id);
            if (id >= 0)
            {
                add_gamepad_input(inputs, id);
            }
        }
        else
        {
            if (find_joystick(id, inputs.curr()) >= 0)
            {
                return;
            }

            id = add_joystick_device(devices.joysticks, id);
            if (id >= 0)
            {
                add_joystick_input(inputs, id);
            }
        }
    }
    
    
    static void open_input_devices(InputDeviceList& devices, input::InputArray& inputs)
    { 
        int num_sdl_joysticks = SDL_NumJoysticks();

        for(int i = 0; i < num_sdl_joysticks; ++i)
        {
            add_input_device(devices, i, inputs);
        }
    }
}


/* static data */

namespace sdl
{
    static InputDeviceList device_list;
}


/* helpers */

namespace sdl
{     
    static f32 normalize_axis_value(Sint16 axis)
    {
        constexpr f32 min = -1.0f;
        constexpr f32 max = 1.0f;

        f32 norm = (f32)axis / 32767;

        return num::abs(norm) < 0.3f ? 0.0f : num::clamp(norm, min, max);
    }


    static void set_unit_vector_state(input::VectorState<i8>& vs, Sint16 x, Sint16 y)
    {
        auto& vec = vs.vec;
        auto& unit = vs.unit;

        vec.x = num::sign_i8(x);
        vec.y = num::sign_i8(y);

        unit.x = (f32)vec.x;
        unit.y = (f32)vec.y;

        constexpr f32 hypot = 1.4142135f;
        constexpr f32 i_hypot = 1.0f / hypot;
        
        auto mag = (x || y) ? 1.0f : 0.0f;
        auto i_mag = (x && y) ? i_hypot : (x || y) ? 1.0f : 0.0f;

        vs.magnitude = mag;
        unit.x *= i_mag;
        unit.y *= i_mag;
    }


    static void set_vector_state(input::VectorState<f32>& vs, Sint16 x, Sint16 y)
    {
        auto& vec = vs.vec;
        auto& unit = vs.unit;

        vec.x = normalize_axis_value(x);
        vec.y = normalize_axis_value(y);

        vs.magnitude = num::magnitude(vec);

        auto mag = vs.magnitude > 0.0f ? vs.magnitude : 1.0f;

        unit.x = vec.x / mag;
        unit.y = vec.y / mag;
    }


    static void handle_window_event(SDL_WindowEvent const& w_event, input::Input& input)
    {
    #ifndef NO_WINDOW

        auto window = SDL_GetWindowFromID(w_event.windowID);

        switch(w_event.event)
        {
            case SDL_WINDOWEVENT_SIZE_CHANGED:
            {
                input.window_size_changed = 1;
            }break;
            case SDL_WINDOWEVENT_EXPOSED:
            {
                
            } break;
        }

    #endif
    }


    static void toggle_fullscreen(SDL_Window* window)
    {
    #ifndef NO_WINDOW

        static bool is_fullscreen = false;

        if (is_fullscreen)
        {
            SDL_SetWindowFullscreen(window, 0);
        }
        else
        {
            SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
        }

        is_fullscreen = !is_fullscreen;

    #endif
    }


    static void handle_sdl_event(SDL_Event const& event, input::Input& input)
    {
        switch(event.type)
        {
        case SDL_WINDOWEVENT:
            handle_window_event(event.window, input);
            break;

        case SDL_QUIT:
            print_message("SDL_QUIT");
            end_program();
            break;

        case SDL_KEYDOWN:
        case SDL_KEYUP:
        {
            auto key_code = event.key.keysym.sym;
            auto alt = event.key.keysym.mod & KMOD_ALT;

            if (alt)
            {
                switch (key_code)
                {
                case SDLK_F4:
                    print_message("ALT F4");
                    end_program();
                    break;

                #ifndef NDEBUG
                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                    print_message("ALT ENTER");
                    auto window = SDL_GetWindowFromID(event.window.windowID);
                    toggle_fullscreen(window);
                    input.window_size_changed = 1;
                    break;
                #endif
                }
            }

            #ifndef NDEBUG

            else
            {
                switch (key_code)
                {
                case SDLK_ESCAPE:
                    print_message("ESC");
                    end_program();
                    break;

                default:
                    break;
                }
            }

            #endif           

        } break;

        default:
            break;
            
        }
    }
}


/* keyboard */

namespace input
{
#ifndef NO_KEYBOARD

    static void record_keyboard_input(SDL_Keycode key_code, KeyboardInput const& old_keyboard, KeyboardInput& new_keyboard, bool is_down)
    {
        switch (key_code)
        {
        #if KEYBOARD_A
        case SDLK_a:
            record_button_input(old_keyboard.kbd_A, new_keyboard.kbd_A, is_down);
            break;
        #endif
        #if KEYBOARD_B
        case SDLK_b:
            record_button_input(old_keyboard.kbd_B, new_keyboard.kbd_B, is_down);
            break;
        #endif
        #if KEYBOARD_C
        case SDLK_c:
            record_button_input(old_keyboard.kbd_C, new_keyboard.kbd_C, is_down);
            break;
        #endif
        #if KEYBOARD_D
        case SDLK_d:
            record_button_input(old_keyboard.kbd_D, new_keyboard.kbd_D, is_down);
            break;
        #endif
        #if KEYBOARD_E
        case SDLK_e:
            record_button_input(old_keyboard.kbd_E, new_keyboard.kbd_E, is_down);
            break;
        #endif
        #if KEYBOARD_F
        case SDLK_f:
            record_button_input(old_keyboard.kbd_F, new_keyboard.kbd_F, is_down);
            break;
        #endif
        #if KEYBOARD_G
        case SDLK_g:
            record_button_input(old_keyboard.kbd_G, new_keyboard.kbd_G, is_down);
            break;
        #endif
        #if KEYBOARD_H
        case SDLK_h:
            record_button_input(old_keyboard.kbd_H, new_keyboard.kbd_H, is_down);
            break;
        #endif
        #if KEYBOARD_I
        case SDLK_i:
            record_button_input(old_keyboard.kbd_I, new_keyboard.kbd_I, is_down);
            break;
        #endif
        #if KEYBOARD_J
        case SDLK_j:
            record_button_input(old_keyboard.kbd_J, new_keyboard.kbd_J, is_down);
            break;
        #endif
        #if KEYBOARD_K
        case SDLK_k:
            record_button_input(old_keyboard.kbd_K, new_keyboard.kbd_K, is_down);
            break;
        #endif
        #if KEYBOARD_L
        case SDLK_l:
            record_button_input(old_keyboard.kbd_L, new_keyboard.kbd_L, is_down);
            break;
        #endif
        #if KEYBOARD_M
        case SDLK_m:
            record_button_input(old_keyboard.kbd_M, new_keyboard.kbd_M, is_down);
            break;
        #endif
        #if KEYBOARD_N
        case SDLK_n:
            record_button_input(old_keyboard.kbd_N, new_keyboard.kbd_N, is_down);
            break;
        #endif
        #if KEYBOARD_O
        case SDLK_o:
            record_button_input(old_keyboard.kbd_O, new_keyboard.kbd_O, is_down);
            break;
        #endif
        #if KEYBOARD_P
        case SDLK_p:
            record_button_input(old_keyboard.kbd_P, new_keyboard.kbd_P, is_down);
            break;
        #endif
        #if KEYBOARD_Q
        case SDLK_q:
            record_button_input(old_keyboard.kbd_Q, new_keyboard.kbd_Q, is_down);
            break;
        #endif
        #if KEYBOARD_R
        case SDLK_r:
            record_button_input(old_keyboard.kbd_R, new_keyboard.kbd_R, is_down);
            break;
        #endif
        #if KEYBOARD_S
        case SDLK_s:
            record_button_input(old_keyboard.kbd_S, new_keyboard.kbd_S, is_down);
            break;
        #endif
        #if KEYBOARD_T
        case SDLK_t:
            record_button_input(old_keyboard.kbd_T, new_keyboard.kbd_T, is_down);
            break;
        #endif
        #if KEYBOARD_U
        case SDLK_u:
            record_button_input(old_keyboard.kbd_U, new_keyboard.kbd_U, is_down);
            break;
        #endif
        #if KEYBOARD_V
        case SDLK_v:
            record_button_input(old_keyboard.kbd_V, new_keyboard.kbd_V, is_down);
            break;
        #endif
        #if KEYBOARD_W
        case SDLK_w:
            record_button_input(old_keyboard.kbd_W, new_keyboard.kbd_W, is_down);
            break;
        #endif
        #if KEYBOARD_X
        case SDLK_x:
            record_button_input(old_keyboard.kbd_X, new_keyboard.kbd_X, is_down);
            break;
        #endif
        #if KEYBOARD_Y
        case SDLK_y:
            record_button_input(old_keyboard.kbd_Y, new_keyboard.kbd_Y, is_down);
            break;
        #endif
        #if KEYBOARD_Z
        case SDLK_z:
            record_button_input(old_keyboard.kbd_Z, new_keyboard.kbd_Z, is_down);
            break;
        #endif
        #if KEYBOARD_0
        case SDLK_0:
            record_button_input(old_keyboard.kbd_0, new_keyboard.kbd_0, is_down);
            break;
        #endif
        #if KEYBOARD_1
        case SDLK_1:
            record_button_input(old_keyboard.kbd_1, new_keyboard.kbd_1, is_down);
            break;
        #endif
        #if KEYBOARD_2
        case SDLK_2:
            record_button_input(old_keyboard.kbd_2, new_keyboard.kbd_2, is_down);
            break;
        #endif
        #if KEYBOARD_3
        case SDLK_3:
            record_button_input(old_keyboard.kbd_3, new_keyboard.kbd_3, is_down);
            break;
        #endif
        #if KEYBOARD_4
        case SDLK_4:
            record_button_input(old_keyboard.kbd_4, new_keyboard.kbd_4, is_down);
            break;
        #endif
        #if KEYBOARD_5
        case SDLK_5:
            record_button_input(old_keyboard.kbd_5, new_keyboard.kbd_5, is_down);
            break;
        #endif
        #if KEYBOARD_6
        case SDLK_6:
            record_button_input(old_keyboard.kbd_6, new_keyboard.kbd_6, is_down);
            break;
        #endif
        #if KEYBOARD_7
        case SDLK_7:
            record_button_input(old_keyboard.kbd_7, new_keyboard.kbd_7, is_down);
            break;
        #endif
        #if KEYBOARD_8
        case SDLK_8:
            record_button_input(old_keyboard.kbd_8, new_keyboard.kbd_8, is_down);
            break;
        #endif
        #if KEYBOARD_9
        case SDLK_9:
            record_button_input(old_keyboard.kbd_9, new_keyboard.kbd_9, is_down);
            break;
        #endif
        #if KEYBOARD_UP
        case SDLK_UP:
            record_button_input(old_keyboard.kbd_up, new_keyboard.kbd_up, is_down);
            break;
        #endif
        #if KEYBOARD_DOWN
        case SDLK_DOWN:
            record_button_input(old_keyboard.kbd_down, new_keyboard.kbd_down, is_down);
            break;
        #endif
        #if KEYBOARD_LEFT
        case SDLK_LEFT:
            record_button_input(old_keyboard.kbd_left, new_keyboard.kbd_left, is_down);
            break;
        #endif
        #if KEYBOARD_RIGHT
        case SDLK_RIGHT:
            record_button_input(old_keyboard.kbd_right, new_keyboard.kbd_right, is_down);
            break;
        #endif
        #if KEYBOARD_RETURN
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
            record_button_input(old_keyboard.kbd_return, new_keyboard.kbd_return, is_down);
            break;
        #endif
        #if KEYBOARD_ESCAPE
        case SDLK_ESCAPE:
            record_button_input(old_keyboard.kbd_escape, new_keyboard.kbd_escape, is_down);
            break;
        #endif
        #if KEYBOARD_SPACE
        case SDLK_SPACE:
            record_button_input(old_keyboard.kbd_space, new_keyboard.kbd_space, is_down);
            break;
        #endif
        #if KEYBOARD_LSHIFT
        case SDLK_LSHIFT:
            record_button_input(old_keyboard.kbd_left_shift, new_keyboard.kbd_left_shift, is_down);
            break;
        #endif
        #if KEYBOARD_RSHIFT
        case SDLK_RSHIFT:
            record_button_input(old_keyboard.kbd_right_shift, new_keyboard.kbd_right_shift, is_down);
            break;
        #endif
        #if KEYBOARD_NUMPAD_0
        case SDLK_KP_0:
            record_button_input(old_keyboard.npd_0, new_keyboard.npd_0, is_down);
            break;
        #endif
        #if KEYBOARD_NUMPAD_1
        case SDLK_KP_1:
            record_button_input(old_keyboard.npd_1, new_keyboard.npd_1, is_down);
            break;
        #endif
        #if KEYBOARD_NUMPAD_2
        case SDLK_KP_2:
            record_button_input(old_keyboard.npd_2, new_keyboard.npd_2, is_down);
            break;
        #endif
        #if KEYBOARD_NUMPAD_3
        case SDLK_KP_3:
            record_button_input(old_keyboard.npd_3, new_keyboard.npd_3, is_down);
            break;
        #endif
        #if KEYBOARD_NUMPAD_4
        case SDLK_KP_4:
            record_button_input(old_keyboard.npd_4, new_keyboard.npd_4, is_down);
            break;
        #endif
        #if KEYBOARD_NUMPAD_5
        case SDLK_KP_5:
            record_button_input(old_keyboard.npd_5, new_keyboard.npd_5, is_down);
            break;
        #endif
        #if KEYBOARD_NUMPAD_6
        case SDLK_KP_6:
            record_button_input(old_keyboard.npd_6, new_keyboard.npd_6, is_down);
            break;
        #endif
        #if KEYBOARD_NUMPAD_7
        case SDLK_KP_7:
            record_button_input(old_keyboard.npd_7, new_keyboard.npd_7, is_down);
            break;
        #endif
        #if KEYBOARD_NUMPAD_8
        case SDLK_KP_8:
            record_button_input(old_keyboard.npd_8, new_keyboard.npd_8, is_down);
            break;
        #endif
        #if KEYBOARD_NUMPAD_9
        case SDLK_KP_9:
            record_button_input(old_keyboard.npd_9, new_keyboard.npd_9, is_down);
            break;
        #endif
        #if KEYBOARD_NUMPAD_PLUS
        case SDLK_KP_PLUS:
            record_button_input(old_keyboard.npd_plus, new_keyboard.npd_plus, is_down);
            break;
        #endif
        #if KEYBOARD_NUMPAD_MINUS
        case SDLK_KP_MINUS:
            record_button_input(old_keyboard.npd_minus, new_keyboard.npd_minus, is_down);
            break;
        #endif
        #if KEYBOARD_NUMPAD_MULTIPLY
        case SDLK_KP_MULTIPLY:
            record_button_input(old_keyboard.npd_mult, new_keyboard.npd_mult, is_down);
            break;
        #endif
        #if KEYBOARD_NUMPAD_DIVIDE
        case SDLK_KP_DIVIDE:
            record_button_input(old_keyboard.npd_div, new_keyboard.npd_div, is_down);
            break;
        #endif
        #if KEYBOARD_CTRL
        case SDLK_LCTRL:
        case SDLK_RCTRL:
            record_button_input(old_keyboard.kbd_ctrl, new_keyboard.kbd_ctrl, is_down);
            break;
        #endif
        
        default:
            break;
        }

        set_is_active(new_keyboard);
    }

#endif

    void record_keyboard_input(SDL_Event const& event, KeyboardInput const& old_keyboard, KeyboardInput& new_keyboard)
    {
    #ifndef NO_KEYBOARD

        switch (event.type)
        {
        case SDL_KEYDOWN:
        case SDL_KEYUP:
        {
            if (event.key.repeat)
            {
                return;
            }

            bool is_down = event.type == SDL_KEYDOWN; //event.key.state == SDL_PRESSED;

            auto key_code = event.key.keysym.sym;
            record_keyboard_input(key_code, old_keyboard, new_keyboard, is_down);
        } break;
        }

    #endif
    }
}


/* mouse */

namespace input
{
#ifndef NO_MOUSE

    static void record_mouse_button_input(Uint8 button_code, MouseInput const& old_mouse, MouseInput& new_mouse, bool is_down)
    {
        switch(button_code)
        {		
        #if MOUSE_LEFT
            case SDL_BUTTON_LEFT:
            {
                record_button_input(old_mouse.btn_left, new_mouse.btn_left, is_down);
            } break;
        #endif
        #if MOUSE_RIGHT
            case SDL_BUTTON_RIGHT:
            {
                record_button_input(old_mouse.btn_right, new_mouse.btn_right, is_down);
            } break;
        #endif
        #if MOUSE_MIDDLE
            case SDL_BUTTON_MIDDLE:
            {
                record_button_input(old_mouse.btn_middle, new_mouse.btn_middle, is_down);
            } break;
        #endif
        #if MOUSE_X1
            case SDL_BUTTON_X1:
            {
                record_button_input(old_mouse.btn_x1, new_mouse.btn_x1, is_down);
            } break;
        #endif
        #if MOUSE_X2
            case SDL_BUTTON_X2:
            {
                record_button_input(old_mouse.btn_x1, new_mouse.btn_x1, is_down);
            } break;
        #endif
        }
    }


    static void record_mouse_position_input(MouseInput& mouse, SDL_MouseMotionEvent const& motion)
    {
    #if MOUSE_POSITION
        mouse.window_pos.x = motion.x;
        mouse.window_pos.y = motion.y;
    #endif
    }


    static void record_mouse_wheel_input(MouseInput& mouse, SDL_MouseWheelEvent const& wheel)
    {
    #if MOUSE_WHEEL
        mouse.wheel.x = wheel.x;
        mouse.wheel.y = wheel.y;
    #endif
    }

#endif

    static void record_mouse_input(SDL_Event const& event, MouseInput const& old_mouse, MouseInput& new_mouse)
    {
    #ifndef NO_MOUSE

        auto& mouse = new_mouse;

        switch (event.type)
        {
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
        {
            bool is_down = event.type == SDL_MOUSEBUTTONDOWN;
            auto button_code = event.button.button;

            record_mouse_button_input(button_code, old_mouse, mouse, is_down);
        } break;

        #if MOUSE_POSITION

        case SDL_MOUSEMOTION:
        {
            record_mouse_position_input(mouse, event.motion);
            mouse.is_active = true;
        } break;

        #endif

        #if MOUSE_WHEEL

        case SDL_MOUSEWHEEL:
        {
            record_mouse_wheel_input(mouse, event.wheel);
            mouse.is_active = true;
        } break;
        #endif
        }

    #endif
    }
}


/* joystick */

namespace input
{
#ifndef NO_JOYSTICK

    void record_joystic_button_input(Uint8 btn_id, JoystickInput const& old_jsk, JoystickInput& new_jsk, bool is_down)
    {
        switch (btn_id)
        {
        #if JOYSTICK_BTN_0
        case 0:
            record_button_input(old_jsk.btn_0, new_jsk.btn_0, is_down);
            break;
        #endif
        #if JOYSTICK_BTN_1
        case 1:
            record_button_input(old_jsk.btn_1, new_jsk.btn_1, is_down);
            break;
        #endif
        #if JOYSTICK_BTN_2
        case 2:
            record_button_input(old_jsk.btn_2, new_jsk.btn_2, is_down);
            break;
        #endif
        #if JOYSTICK_BTN_3
        case 3:
            record_button_input(old_jsk.btn_3, new_jsk.btn_3, is_down);
            break;
        #endif
        #if JOYSTICK_BTN_4
        case 4:
            record_button_input(old_jsk.btn_4, new_jsk.btn_4, is_down);
            break;
        #endif
        #if JOYSTICK_BTN_5
        case 5:
            record_button_input(old_jsk.btn_5, new_jsk.btn_5, is_down);
            break;
        #endif
        #if JOYSTICK_BTN_6
        case 6:
            record_button_input(old_jsk.btn_6, new_jsk.btn_6, is_down);
            break;
        #endif
        #if JOYSTICK_BTN_7
        case 7:
            record_button_input(old_jsk.btn_7, new_jsk.btn_7, is_down);
            break;
        #endif
        #if JOYSTICK_BTN_8
        case 8:
            record_button_input(old_jsk.btn_8, new_jsk.btn_8, is_down);
            break;
        #endif
        #if JOYSTICK_BTN_9
        case 9:
            record_button_input(old_jsk.btn_9, new_jsk.btn_9, is_down);
            break;
        #endif
        }
    }


    static void record_joystick_axis_input(Uint8 axis_id, JoystickInput& jsk, Sint16 axis_value)
    {
        auto val32 = sdl::normalize_axis_value(axis_value);

        switch (axis_id)
        {        
        #if JOYSTICK_AXIS_0
        case 0:
            jsk.axis_0 = val32;
            break;
        #endif
        #if JOYSTICK_AXIS_1
        case 1:
            jsk.axis_1 = val32;
            break;
        #endif
        #if JOYSTICK_AXIS_2
        case 2:
            jsk.axis_2 = val32;
            break;
        #endif
        #if JOYSTICK_AXIS_3
        case 3:
            jsk.axis_3 = val32;
            break;
        #endif
        #if JOYSTICK_AXIS_4
        case 4:
            jsk.axis_4 = val32;
            break;
        #endif
        #if JOYSTICK_AXIS_5
        case 5:
            jsk.axis_5 = val32;
            break;
        #endif

        default:
            break;
        }
    }

#endif

    static void record_joystick_input(SDL_Event const& event, Input const& prev, Input& curr)
    {
    #ifndef NO_JOYSTICK

        int id = -1;

        Uint8 btn = 255;
        bool is_down = false;
        
        Uint8 axis = 255;
        Sint16 value = 0;

        switch (event.type)
        {
        case SDL_JOYBUTTONDOWN:
        case SDL_JOYBUTTONUP:
        {
            is_down = event.type == SDL_JOYBUTTONDOWN;
            btn = event.jbutton.button;

            id = sdl::find_joystick(event.jbutton.which, curr);
            if (id >= 0)
            {
                record_joystic_button_input(btn, prev.joysticks[id], curr.joysticks[id], is_down);
            }

        } break;

        case SDL_JOYAXISMOTION:
        {
            axis = event.jaxis.axis;
            value = event.jaxis.value;

            id = sdl::find_joystick(event.jaxis.which, curr);
            if (id >= 0)
            {
                record_joystick_axis_input(axis, curr.joysticks[id], value);
            }

        } break;

        default:
            break;
        }

    #endif
    }


}


/* gamepad */

namespace input
{ 
#ifndef NO_GAMEPAD

    static f32 get_gamepad_axis(SDL_GameController* sdl_gamepad, SDL_GameControllerAxis axis_key)
    {
        auto axis = SDL_GameControllerGetAxis(sdl_gamepad, axis_key);
        return sdl::normalize_axis_value(axis);
    }


    static void record_gamepad_button(SDL_GameController* sdl_gamepad, SDL_GameControllerButton btn_key, ButtonState const& old_btn, ButtonState& new_btn)
    {
        auto is_down = SDL_GameControllerGetButton(sdl_gamepad, btn_key);
        record_button_input(old_btn, new_btn, is_down);
    }


    static void record_gamepad_stick_button(SDL_GameController* sdl_gamepad, SDL_GameControllerButton btn_key, ButtonState const& old_btn, ButtonState& new_btn, VectorState<f32> const& stick)
    {
        // ignore button press if stick is used for direction
        auto is_down = SDL_GameControllerGetButton(sdl_gamepad, btn_key) && stick.magnitude < 0.3;
        record_button_input(old_btn, new_btn, is_down);
    }


    static void record_gamepad_button_input(SDL_GameController* sdl_gamepad, GamepadInput const& old_gamepad, GamepadInput& new_gamepad)
    {

    #if GAMEPAD_BTN_DPAD_UP
        record_gamepad_button(sdl_gamepad, SDL_CONTROLLER_BUTTON_DPAD_UP, old_gamepad.btn_dpad_up, new_gamepad.btn_dpad_up);
    #endif
    #if GAMEPAD_BTN_DPAD_DOWN
        record_gamepad_button(sdl_gamepad, SDL_CONTROLLER_BUTTON_DPAD_DOWN, old_gamepad.btn_dpad_down, new_gamepad.btn_dpad_down);
    #endif
    #if GAMEPAD_BTN_DPAD_LEFT
        record_gamepad_button(sdl_gamepad, SDL_CONTROLLER_BUTTON_DPAD_LEFT, old_gamepad.btn_dpad_left, new_gamepad.btn_dpad_left);
    #endif
    #if GAMEPAD_BTN_DPAD_RIGHT
        record_gamepad_button(sdl_gamepad, SDL_CONTROLLER_BUTTON_DPAD_RIGHT, old_gamepad.btn_dpad_right, new_gamepad.btn_dpad_right);
    #endif
    #if GAMEPAD_BTN_START
        record_gamepad_button(sdl_gamepad, SDL_CONTROLLER_BUTTON_START, old_gamepad.btn_start, new_gamepad.btn_start);
    #endif
    #if GAMEPAD_BTN_BACK
        record_gamepad_button(sdl_gamepad, SDL_CONTROLLER_BUTTON_BACK, old_gamepad.btn_back, new_gamepad.btn_back);
    #endif
    #if GAMEPAD_BTN_SOUTH
        record_gamepad_button(sdl_gamepad, SDL_CONTROLLER_BUTTON_A, old_gamepad.btn_south, new_gamepad.btn_south);
    #endif
    #if GAMEPAD_BTN_EAST
        record_gamepad_button(sdl_gamepad, SDL_CONTROLLER_BUTTON_B, old_gamepad.btn_east, new_gamepad.btn_east);
    #endif
    #if GAMEPAD_BTN_WEST
        record_gamepad_button(sdl_gamepad, SDL_CONTROLLER_BUTTON_X, old_gamepad.btn_west, new_gamepad.btn_west);
    #endif
    #if GAMEPAD_BTN_NORTH
        record_gamepad_button(sdl_gamepad, SDL_CONTROLLER_BUTTON_Y, old_gamepad.btn_north, new_gamepad.btn_north);
    #endif
    #if GAMEPAD_BTN_SHOULDER_LEFT
        record_gamepad_button(sdl_gamepad, SDL_CONTROLLER_BUTTON_LEFTSHOULDER, old_gamepad.btn_shoulder_left, new_gamepad.btn_shoulder_left);
    #endif
    #if GAMEPAD_BTN_SHOULDER_RIGHT
        record_gamepad_button(sdl_gamepad, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, old_gamepad.btn_shoulder_right, new_gamepad.btn_shoulder_right);
    #endif
    #if GAMEPAD_BTN_STICK_LEFT
        record_gamepad_stick_button(sdl_gamepad, SDL_CONTROLLER_BUTTON_LEFTSTICK, old_gamepad.btn_stick_left, new_gamepad.btn_stick_left, new_gamepad.stick_left);
    #endif
    #if GAMEPAD_BTN_STICK_RIGHT
        record_gamepad_stick_button(sdl_gamepad, SDL_CONTROLLER_BUTTON_RIGHTSTICK, old_gamepad.btn_stick_right, new_gamepad.btn_stick_right, new_gamepad.stick_right);        
    #endif

    }


    static void record_gamepad_dpad_vector(GamepadInput& gamepad)
    {
    #if GAMEPAD_BTN_DPAD_ALL

        Sint16 x = (Sint16)gamepad.btn_dpad_right.is_down - (Sint16)gamepad.btn_dpad_left.is_down;
        Sint16 y = (Sint16)gamepad.btn_dpad_down.is_down - (Sint16)gamepad.btn_dpad_up.is_down;

        sdl::set_unit_vector_state(gamepad.vec_dpad, x, y);

    #endif
    }


    static void record_gamepad_trigger_input(SDL_GameController* sdl_gamepad, GamepadInput& gamepad)
    {

    #if GAMEPAD_TRIGGER_LEFT
        gamepad.trigger_left = get_gamepad_axis(sdl_gamepad, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
    #endif
    #if GAMEPAD_TRIGGER_RIGHT
        gamepad.trigger_right = get_gamepad_axis(sdl_gamepad, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
    #endif

    }


    static void record_gamepad_axis_input(SDL_GameController* sdl_gamepad, GamepadInput& gamepad)
    {
        Sint16 x = 0;
        Sint16 y = 0;

    #if GAMEPAD_AXIS_STICK_LEFT

        x = SDL_GameControllerGetAxis(sdl_gamepad, SDL_CONTROLLER_AXIS_LEFTX);
        y = SDL_GameControllerGetAxis(sdl_gamepad, SDL_CONTROLLER_AXIS_LEFTY);

        sdl::set_vector_state(gamepad.stick_left, x, y);

    #endif
    #if GAMEPAD_AXIS_STICK_RIGHT
        
        x = SDL_GameControllerGetAxis(sdl_gamepad, SDL_CONTROLLER_AXIS_RIGHTX);
        y = SDL_GameControllerGetAxis(sdl_gamepad, SDL_CONTROLLER_AXIS_RIGHTY);

        sdl::set_vector_state(gamepad.stick_right, x, y);
        
    #endif
    }


    static void record_gamepad_gamepad_input(SDL_GameController* sdl_gamepad, GamepadInput const& old_gamepad, GamepadInput& new_gamepad)
    {
        if(!sdl_gamepad || !SDL_GameControllerGetAttached(sdl_gamepad))
        {
            return;
        }

        record_gamepad_button_input(sdl_gamepad, old_gamepad, new_gamepad);
        record_gamepad_dpad_vector(new_gamepad);        
        record_gamepad_trigger_input(sdl_gamepad, new_gamepad);
        record_gamepad_axis_input(sdl_gamepad, new_gamepad);        
    }

#endif


    static void record_gamepad_input(Input const& prev, Input& curr)
    {
    #ifndef NO_GAMEPAD
        i32 id = -1;

        static_assert(MAX_GAMEPADS == sdl::device_list.gamepads.capacity);

        auto& gamepads = sdl::device_list.gamepads;

        for (u32 i = 0; i < gamepads.capacity; i++)
        {
            if (!gamepads.data[i].gamepad)
            {
                continue;
            }

            id = sdl::find_gamepad(gamepads.data[i].id, prev);
            if (id < 0)
            {
                continue;
            }

            auto& p = prev.gamepads[id];
            auto& c = curr.gamepads[id];

            record_gamepad_gamepad_input(gamepads.data[i].gamepad, p, c);
            set_is_active(c);
        }

    #endif
    }
}


/* add remove */

namespace input
{
    static void update_device_list(SDL_Event const& event, input::InputArray& inputs)
    {
        switch (event.type)
        {

        case SDL_JOYDEVICEADDED:
        {
            sdl::add_input_device(sdl::device_list, event.jdevice.which, inputs);                        

        } break;

        case SDL_JOYDEVICEREMOVED:
        {
            sdl::remove_input_device(sdl::device_list, event.jdevice.which, inputs);
        } break;

        default:
            break;
        }
    }

}


/* api */

namespace input
{
    static constexpr Uint32 subsystem_flags()
    {
        Uint32 flags = SDL_INIT_GAMECONTROLLER;
        flags |= SDL_INIT_JOYSTICK;

    #ifndef NO_HAPTIC
        flags |= SDL_INIT_HAPTIC;
    #endif

        return flags;
    }


    bool init(InputArray& inputs)
    {
        reset_input_state(inputs.prev());
        reset_input_state(inputs.curr());

        auto error = SDL_InitSubSystem(subsystem_flags());
        if (error)
        {
            sdl::print_error("Init Input failed");
            return false;
        }

        sdl::open_input_devices(sdl::device_list, inputs);

        return true;
    }


    void close()
    {
        sdl::close_input_devices(sdl::device_list);
        SDL_QuitSubSystem(subsystem_flags());
    }


    void record_input(InputArray& inputs)
    {
        auto& prev = inputs.prev();
        auto& curr = inputs.curr();

        copy_input_state(prev, curr);
        curr.frame = prev.frame + 1;
        curr.dt_frame = 1.0f / 60.0f; // TODO
        curr.window_size_changed = 0;

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            sdl::handle_sdl_event(event, curr);
            record_keyboard_input(event, prev.keyboard, curr.keyboard);
            record_mouse_input(event, prev.mouse, curr.mouse);
            update_device_list(event, inputs);
            record_joystick_input(event, prev, curr);
        }

        record_gamepad_input(prev, curr);

        set_is_active(curr);
    }


    void record_input(InputArray& inputs, event_cb handle_event)
    {
        auto& prev = inputs.prev();
        auto& curr = inputs.curr();

        copy_input_state(prev, curr);
        curr.frame = prev.frame + 1;
        curr.dt_frame = 1.0f / 60.0f; // TODO
        curr.window_size_changed = 0;

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            handle_event(&event);
            sdl::handle_sdl_event(event, curr);
            record_keyboard_input(event, prev.keyboard, curr.keyboard);
            record_mouse_input(event, prev.mouse, curr.mouse);
            update_device_list(event, inputs);
            record_joystick_input(event, prev, curr);
        }

        record_gamepad_input(prev, curr);

        set_is_active(curr);
    }
}