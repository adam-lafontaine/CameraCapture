#include "../../imgui_sdl_ogl/imgui_include.hpp"
#include "../../input_display/input_display.hpp"
#include "../../camera_display/camera_display.hpp"
#include "../../diagnostics/diagnostics.hpp"
#include "../../../../libs/sdl/sdl_include.hpp"
#include "../../../../libs/util/stopwatch.hpp"


#include <thread>


namespace img = image;
namespace idsp = input_display;
namespace cdsp = camera_display;


constexpr f64 NANO = 1'000'000'000;
constexpr f64 MICRO = 1'000'000;

constexpr f64 TARGET_FRAMERATE_HZ = 60.0;
constexpr f64 TARGET_NS_PER_FRAME = NANO / TARGET_FRAMERATE_HZ;



static void set_game_window_icon(SDL_Window* window)
{
#include "../../../../resources/icon_64.c" // this will "paste" the struct my_icon into this function
    sdl::set_window_icon(window, icon_64);
}


static void ui_process_input(sdl::EventInfo& evt, input::Input const& prev, input::Input& curr, ui::UIState& state)
{
    //auto& io = *state.io;

    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    
    /*if (!io.WantCaptureKeyboard)
    {
        evt.has_event = true;
        input::process_keyboard_input(evt, prev.keyboard, curr.keyboard);
        evt.first_in_queue = false;
    }

    if (!io.WantCaptureMouse)
    {
        evt.has_event = true;
        input::process_mouse_input(evt, prev.mouse, curr.mouse);
        evt.first_in_queue = false;
    }*/
}


static void ui_input_window(GLuint texture, u32 width, u32 height, f32 scale)
{
    auto w = width * scale;
    auto h = height * scale;

    ImGui::Begin("Input");

    ImGui::Image((void*)(intptr_t)texture, ImVec2(w, h));

    ImGui::End();
}


static void ui_camera_window(GLuint texture, u32 width, u32 height, f32 scale)
{
    auto w = width * scale;
    auto h = height * scale;

    ImGui::Begin("Camera");

    ImGui::Image((void*)(intptr_t)texture, ImVec2(w, h));

    ImGui::End();
}


static void ui_camera_controls_window(cdsp::CameraState& state)
{
    ImGui::Begin("Controls");

    camera_display::show_cameras(state);

    ImGui::End();
}


static void ui_diagnostics_window()
{
    ImGui::Begin("Diagnostics");

    diagnostics::show_memory();
    diagnostics::show_uvc_memory();

    ImGui::End();
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
    input::Input user_input[2] = {};
    sdl::ControllerInput sdl_controller = {};
    u8 input_id_curr = 0;
    u8 input_id_prev = 1;

    idsp::IOState io_state{};
    cdsp::CameraState camera_state{};

    img::Buffer32 camera_buffer;
    

    constexpr u32 N_OGL_TEXTURES = 2;
    constexpr ogl::TextureId input_texture_id = { 0 };
    constexpr ogl::TextureId camera_texture_id = { 1 };
    ogl::TextureList<N_OGL_TEXTURES> textures;

    ui::UIState ui_state{};
    SDL_Window* window = 0;
    SDL_GLContext gl_context;
    
    RunState run_state = RunState::Begin;

    Stopwatch main_sw;
    f64 main_frame_ns;
}


static void end_program()
{
    run_state = RunState::End;
}


static bool is_running()
{
    return run_state != RunState::End;
}


static void init_input()
{
    sdl::open_game_controllers(sdl_controller, user_input[0]);
    user_input[1].num_controllers = user_input[0].num_controllers;
    user_input[0].frame = user_input[1].frame = (u64)0 - 1;
}


static void swap_inputs()
{
    input_id_prev = input_id_curr;
    input_id_curr = !input_id_curr;

    auto& input = user_input[input_id_curr];
    auto& input_prev = user_input[input_id_prev];

    input::copy_input(input_prev, input);
}


static bool run_state_changed()
{
    bool updated = false;

    return updated;
}


static void wait_for_framerate()
{
    constexpr f64 fudge = 0.9;

    main_frame_ns = main_sw.get_time_nano();
    auto sleep_ns = TARGET_NS_PER_FRAME - main_frame_ns;
    if (sleep_ns > 0.0)
    {
        std::this_thread::sleep_for(std::chrono::nanoseconds((i64)(sleep_ns * fudge)));
    }

    main_sw.start();
}


static void handle_window_event(SDL_Event const& event, SDL_Window* window)
{
    switch (event.type)
    {
    case SDL_QUIT:
        end_program();
        break;

    case SDL_WINDOWEVENT:
    {
        switch (event.window.event)
        {
        case SDL_WINDOWEVENT_SIZE_CHANGED:
            int w, h;
            SDL_GetWindowSize(window, &w, &h);
            glViewport(0, 0, w, h);
            break;

        case SDL_WINDOWEVENT_CLOSE:
            end_program();
            break;
        
        default:
            break;
        }
    } break;

    case SDL_KEYDOWN:
    case SDL_KEYUP:
    {
        auto key_code = event.key.keysym.sym;
        switch (key_code)
        {
        case SDLK_ESCAPE:
            sdl::print_message("ESC");
            end_program();
            break;

        default:
            break;
        }

    } break;
    
    default:
        break;
    }
}


static void process_user_input()
{
    swap_inputs();

    auto& input = user_input[input_id_curr];
    auto& input_prev = user_input[input_id_prev];

    input.frame = input_prev.frame + 1;

    sdl::EventInfo evt{};
    evt.has_event = false;

    // Poll and handle events (inputs, window resize, etc.)
    while (SDL_PollEvent(&evt.event))
    {
        handle_window_event(evt.event, window);
        ImGui_ImplSDL2_ProcessEvent(&evt.event);

        evt.has_event = true;

        input::process_keyboard_input(evt, input_prev.keyboard, input.keyboard);
        input::process_mouse_input(evt, input_prev.mouse, input.mouse);        
    }

    input::process_controller_input(sdl_controller, input_prev, input);

    ui_process_input(evt, input_prev, input, ui_state);

    
}


static void render_imgui_frame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // Rendering
    ImGui::DockSpaceOverViewport(nullptr, ImGuiDockNodeFlags_None);

#ifdef SHOW_IMGUI_DEMO
    ui::show_imgui_demo(ui_state);
#endif

    ui_input_window(textures.data[input_texture_id.value], io_state.display.width, io_state.display.height, 2.0f);
    ui_camera_window(textures.data[camera_texture_id.value], camera_state.display.width, camera_state.display.height, 1.0f);
    ui_camera_controls_window(camera_state);

    ui_diagnostics_window();

    ImGui::Render();
    
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    if ((*ui_state.io).ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
        SDL_GL_MakeCurrent(window, gl_context);
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        SDL_GL_MakeCurrent(backup_current_window, gl_context);
    }

    SDL_GL_SwapWindow(window);    
}


static void render_input_display(img::ImageView const& src)
{
    ogl::render_to_texture(src.matrix_data_, (int)src.width, (int)src.height, input_texture_id);
}


static void render_camera(img::ImageView const& src)
{
    ogl::render_to_texture(src.matrix_data_, (int)src.width, (int)src.height, camera_texture_id);
}


static bool main_init()
{
    if(!sdl::init())
    {       
        sdl::print_message("Error: sdl::init()"); 
        return false;
    }

    // From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    // fullscreen window
    SDL_DisplayMode dm;
    SDL_GetCurrentDisplayMode(0, &dm);

    window = ui::create_sdl_ogl_window("Camera", dm.w, dm.h);    
    if (!window)
    {
        sdl::print_error("Error: create_sdl_ogl_window()");
        sdl::close();
        return false;
    }

    set_game_window_icon(window);

    init_input();        

    gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;    

    // Setup Platform/Renderer backends
    auto glsl_version = ogl::get_glsl_version();
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    ui::set_imgui_style();

    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    
    ui_state.io = &io;

    textures = ogl::create_textures<N_OGL_TEXTURES>();
    
    if (!idsp::init(io_state))
    {
        sdl::print_message("Error: idsp::init()");
        sdl::close();
        return false;
    }

    

    // TODO init camera app
    u32 w = 1280;
    u32 h = 720;
    camera_buffer = img::create_buffer32(w * h, "camera display");
    camera_state.display = img::make_view(w, h, camera_buffer);
    img::fill(camera_state.display, img::to_pixel(128));

    cdsp::init_async(camera_state);

    return true;
}


static void main_close()
{ 
    idsp::close(io_state);
    cdsp::close(camera_state);
    
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl_context);

    sdl::close_game_controllers(sdl_controller, user_input[0]);
    sdl::close();

    mb::destroy_buffer(camera_buffer);
}


static void main_loop()
{
    main_sw.start();
    
    while(is_running())
    {
        process_user_input();

        if (run_state_changed())
        {
            return;
        }

        auto& input = user_input[input_id_curr];
        
        idsp::update(input, io_state);
        render_input_display(io_state.display);

        render_camera(camera_state.display);

        render_imgui_frame();        

        // cap frame rate
        wait_for_framerate();
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

    main_close();

    return EXIT_SUCCESS;
}