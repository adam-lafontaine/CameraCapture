#include "../../../../libs/sdl/sdl_include.hpp"
#include "../../../../libs/util/stopwatch.hpp"
#include "../../../../libs/qsprintf/qsprintf.hpp"
#include "../../app/app.hpp"

#include <thread>
#include <cassert>

namespace mb = memory_buffer;
namespace img = image;

constexpr auto WINDOW_TITLE = "camera_usb test";
constexpr u32 WINDOW_SCALE = 1;

constexpr f64 NANO = 1'000'000'000;

constexpr f64 TARGET_FRAMERATE_HZ = 60.0;
constexpr f64 TARGET_NS_PER_FRAME = NANO / TARGET_FRAMERATE_HZ;


static void cap_framerate(Stopwatch& sw, f64 target_ns)
{
    constexpr f64 fudge = 0.9;

    auto sleep_ns = target_ns - sw.get_time_nano();
    if (sleep_ns > 0)
    {
        std::this_thread::sleep_for(std::chrono::nanoseconds((i64)(sleep_ns * fudge)));
    }

    sw.start();
}


enum class RunState : int
{
    Begin,
    Run,
    End
};


/* main variables */

namespace
{    
    app::AppState app_state{};
    sdl::ScreenMemory sdl_screen{};
    input::Input user_input[2] = {};
    sdl::ControllerInput sdl_controller = {};
    b32 input_id_curr = 0;
    b32 input_id_prev = 1;

    RunState run_state = RunState::Begin;

#ifndef NDEBUG
    f64 dbg_frame_nano = TARGET_NS_PER_FRAME;
    f64 dbg_ns_elapsed = 0.0;
    constexpr f64 dbg_title_refresh_ns = NANO * 0.25;
    constexpr int dbg_TITLE_LEN = 50;
    char dbg_title[dbg_TITLE_LEN] = { 0 };
    int dbg_frame_milli = 0;
#endif
}


static void end_program()
{
    run_state = RunState::End;
}


static inline void swap_inputs()
{
    input_id_prev = input_id_curr;
    input_id_curr = !input_id_curr;
}


static inline bool is_running()
{
    return run_state != RunState::End;
}


static void handle_sdl_event(SDL_Event const& event, SDL_Window* window)
{
    switch(event.type)
    {
    case SDL_WINDOWEVENT:
        sdl::handle_window_event(event.window);
        break;

    case SDL_QUIT:
        sdl::print_message("SDL_QUIT");
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
                sdl::print_message("ALT F4");
                end_program();
                break;

#ifndef NDEBUG

            case SDLK_RETURN:
            case SDLK_KP_ENTER:
                sdl::print_message("ALT ENTER");
                sdl::toggle_fullscreen(window);
                break;

#endif

            
            default:
                break;
            }
        }


#ifndef NDEBUG

        else
        {
            switch (key_code)
            {
            case SDLK_ESCAPE:
                sdl::print_message("ESC");
                end_program();
                break;

            default:
                break;
            }
        }

#endif           

    } break;
        
    }
}


static void process_input()
{
    sdl::EventInfo evt{};
    evt.has_event = false;

    auto& input_curr = user_input[input_id_curr];
    auto& input_prev = user_input[input_id_prev];

    input::copy_input(input_prev, input_curr);
    input_curr.frame = input_prev.frame + 1;
    input_curr.dt_frame = (f32)(1.0 / TARGET_FRAMERATE_HZ); // does not miss frames but slows animation        

    while (SDL_PollEvent(&evt.event))
    {
        evt.has_event = true;
        handle_sdl_event(evt.event, sdl_screen.window);
        input::process_keyboard_input(evt, input_prev.keyboard, input_curr.keyboard);
        input::process_mouse_input(evt, input_prev.mouse, input_curr.mouse);
    }

    input::process_controller_input(sdl_controller, input_prev, input_curr);
}


static bool main_init()
{
    if(!sdl::init())
    {        
        return false;
    }
    
    auto result = app::init(app_state);
    if (!result.success)
    {
        sdl::print_message("Error: app::init()");
        sdl::close();
        return false;
    }

    auto app_dims = result.screen_dimensions;
    
    Vec2Du32 window_dims = {
        WINDOW_SCALE * app_dims.x,
        WINDOW_SCALE * app_dims.y
    };
    
    if(!sdl::create_screen_memory(sdl_screen, WINDOW_TITLE, app_dims, window_dims))
    {
        sdl::print_message("Error: sdl::create_screen_memory()");
        sdl::close();
        return false;
    }

    if (!app::set_screen_memory(app_state, sdl_screen.view))
    {
        sdl::print_message("Error: set_screen_memory()");
        sdl::close();
        return false;
    }
    
    sdl::open_game_controllers(sdl_controller, user_input[0]);
    user_input[1].num_controllers = user_input[0].num_controllers;
    user_input[0].frame = user_input[1].frame = (u64)0 - 1;

    //set_window_icon(sdl_screen);
    img::fill(sdl_screen.view, img::to_pixel(0));

    return true;
}


static void main_loop()
{
    Stopwatch sw;
    sw.start();
    while(is_running())
    {
        process_input();

        app::update(app_state, user_input[input_id_curr]);

        sdl::render_screen(sdl_screen); 

#ifndef NDEBUG
        dbg_frame_nano = sw.get_time_nano();
        dbg_frame_milli = (int)(dbg_frame_nano / 1'000'000 + 0.5);
        
        dbg_ns_elapsed += dbg_frame_nano;
        if(dbg_ns_elapsed >= dbg_title_refresh_ns)
        {
            qsnprintf(dbg_title, dbg_TITLE_LEN, "%s (%d ms)", WINDOW_TITLE, dbg_frame_milli);
            SDL_SetWindowTitle(sdl_screen.window, dbg_title);

            dbg_ns_elapsed = 0.0;
        }
#endif

        swap_inputs();
        cap_framerate(sw, TARGET_NS_PER_FRAME);
    }
}


int main()
{
    if (!main_init())
    {
        return EXIT_FAILURE;
    }

    run_state = RunState::Run;

    while (is_running())
    {
        main_loop();
    }

    sdl::close_game_controllers(sdl_controller, user_input[0]);
    sdl::destroy_screen_memory(sdl_screen);
    sdl::close();

    return EXIT_SUCCESS;
}

#include "../../app/app.cpp"
#include "../../../../libs/sdl/sdl_input.cpp"
#include "../../../../libs/qsprintf/qsprintf.cpp"