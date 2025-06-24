#include "../imgui_sdl2_dx11/imgui_include.hpp"
#include "../../camera_display/camera_display.hpp"

#ifndef NDEBUG
#include "../../diagnostics/diagnostics.hpp"
#endif


namespace img = image;
namespace cdsp = camera_display;



static void set_game_window_icon(SDL_Window* window)
{
#include "../../../../resources/icon_64.c" // this will "paste" the struct my_icon into this function
    sdl::set_window_icon(window, icon_64);
}


static void texture_window(cstr title, ImTextureID image_texture, u32 width, u32 height, f32 scale)
{
    auto w = width * scale;
    auto h = height * scale;

    ImGui::Begin(title);

    ImGui::Image(texture, ImVec2(w, h));

    ImGui::End();
}


static void ui_camera_controls_window(cdsp::CameraState& state)
{
    ImGui::Begin("Controls");

    camera_display::show_cameras(state);

    ImGui::End();
}


enum class RunState : int
{
    Begin,
    Run,
    End
};


/* main variables */

namespace mv
{    
    RunState run_state = RunState::Begin;
    ui_imgui::UIState ui_state{};

    constexpr u32 N_TEXTURES = 1;
    
    cdsp::CameraState camera_state{};
    img::Buffer32 camera_buffer;
    
    constexpr dx11::TextureId camera_texture_id = { 0 };
}


static bool init_input_display()
{
#ifndef NDEBUG

    if (!idsp::init(io_state))
    {
        sdl::print_message("Error: idsp::init()");
        sdl::close();
        return false;
    }
    
    auto data = io_state.display.matrix_data_;
    auto w = io_state.display.width;
    auto h = io_state.display.height;

    dx11::init_texture(data, w, h, textures.get(input_texture_id), dx_ctx);
#endif

    return true;
}


static void init_camera_display()
{
    // TODO init camera app
    u32 w = 640;
    u32 h = 480;
    camera_buffer = img::create_buffer32(w * h, "camera display");
    camera_state.display = img::make_view(w, h, camera_buffer);
    img::fill(camera_state.display, img::to_pixel(128));

    cdsp::init_async(camera_state);

    auto data = camera_state.display.matrix_data_;
    
    dx11::init_texture(data, w, h, textures.get(camera_texture_id), dx_ctx);
}


static void end_program()
{
    run_state = RunState::End;
}


static bool is_running()
{
    return run_state != RunState::End;
}


static void handle_window_event(SDL_Event const& event, SDL_Window* window)
{
    auto const window_resize = [&]()
    {
        if (event.window.windowID == SDL_GetWindowID(window))
        {
            return;
        }

        // TODO
        int w, h;
        SDL_GetWindowSize(window, &w, &h);

        // Release all outstanding references to the swap chain's buffers before resizing.
        dx11::cleanup_render_target(dx_ctx);
        dx_ctx.pSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
        dx11::create_render_target(dx_ctx);
    };

    auto const window_close = [&]()
    {
        if (event.window.windowID == SDL_GetWindowID(window))
        {
            end_program();
        }
    };

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
        case SDL_WINDOWEVENT_RESIZED:
            window_resize();
            break;

        case SDL_WINDOWEVENT_CLOSE:
            window_close();
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


static void render_imgui_frame()
{
    ui_imgui::new_frame();
    ui_imgui::show_imgui_demo(mv::ui_state);
    
    auto t = mv::textures.get_im_texture_id(mv::camera_texture_id);
    auto w = mv::camera_state.display.width;
    auto h = mv::camera_state.display.height;
    auto scale = 1.0f;
    texture_window("Camera", t, w, h, scale);

    ui_camera_controls_window(mv::camera_state);

    ui_imgui::render(mv::ui_state);    
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

    auto df = 0.9f;
    auto dw = (int)(df * dm.w);
    auto dh = (int)(df * dm.h);

    //auto dw = dm.w;
    //auto dh = dm.h;

    window = ui::create_sdl_dx11_window("Camera", dw, dh);
    if (!window)
    {
        sdl::print_error("Error: create_sdl_ogl_window()");
        sdl::close();
        return false;
    }

    set_game_window_icon(window);

    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(window, &wmInfo);
    HWND hwnd = (HWND)wmInfo.info.win.window;

    // Initialize Direct3D
    if (!dx11::init_context(dx_ctx, hwnd))
    {
        dx11::close_context(dx_ctx);
        return false;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForD3D(window);
    ImGui_ImplDX11_Init(dx_ctx.pd3dDevice, dx_ctx.pd3dDeviceContext);

    ui::set_imgui_style();
    ui_state.io = &io;

    textures = dx11::create_textures<N_TEXTURES>();

    init_camera_display();

    if (!init_input_display())
    {
        return false;
    }

    return true;
}


static void main_close()
{
    cdsp::close_async(camera_state);

#ifndef NDEBUG
    idsp::close(io_state);
#endif    

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    dx11::close_context(dx_ctx);

    SDL_DestroyWindow(window);

    sdl::close_game_controllers(sdl_controller, user_input[0]);
    sdl::close();

    mb::destroy_buffer(camera_buffer);
}


static void main_loop()
{
    init_input();
    
    while(is_running())
    {
        process_user_input();

        auto& input = user_input[input_id_curr];

#ifndef NDEBUG        
        idsp::update(input, io_state);
        dx11::render_texture(textures.get(input_texture_id), dx_ctx);
#endif 
        dx11::render_texture(textures.get(camera_texture_id), dx_ctx);

        render_imgui_frame(); 
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

#include "main_o.cpp"