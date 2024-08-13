#include "imgui_include.hpp"
#include "../../input_display/input_display.hpp"
#include "../../camera_display/camera_display.hpp"
#include "../../diagnostics/diagnostics.hpp"
#include "../../../../libs/sdl/sdl_include.hpp"
#include "../../../../libs/util/stopwatch.hpp"


namespace img = image;
namespace idsp = input_display;
namespace cdsp = camera_display;



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


/*static void ui_input_window(GLuint texture, u32 width, u32 height, f32 scale)
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
}*/


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

    ui::UIState ui_state{};
    SDL_Window* window = 0;
    
    RunState run_state = RunState::Begin;

    Stopwatch main_sw;
    f64 main_frame_ns;
}


// Data
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();


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
        CleanupRenderTarget();
        g_pSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
        CreateRenderTarget();
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
    // Start the Dear ImGui frame
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // Rendering
    ImGui::DockSpaceOverViewport(nullptr, ImGuiDockNodeFlags_None);

#ifdef SHOW_IMGUI_DEMO
    ui::show_imgui_demo(ui_state);
#endif

    //ui_input_window(textures.data[input_texture_id.value], io_state.display.width, io_state.display.height, 2.0f);
    //ui_camera_window(textures.data[camera_texture_id.value], camera_state.display.width, camera_state.display.height, 1.0f);
    ui_camera_controls_window(camera_state);

    ui_diagnostics_window();

    ImGui::Render();

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
    g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    g_pSwapChain->Present(1, 0); // Present with vsync
    //g_pSwapChain->Present(0, 0); // Present without vsync
}


static void render_input_display(img::ImageView const& src)
{

}


static void render_camera(img::ImageView const& src)
{

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

    //auto dw = (int)(0.8f * dm.w);
    //auto dh = (int)(0.8f * dm.h);

    auto dw = dm.w;
    auto dh = dm.h;

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
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        return 1;
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
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    ui::set_imgui_style();
    ui_state.io = &io;

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

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();

    SDL_DestroyWindow(window);

    sdl::close_game_controllers(sdl_controller, user_input[0]);
    sdl::close();

    mb::destroy_buffer(camera_buffer);
}


static void main_loop()
{
    init_input();
    main_sw.start();
    
    while(is_running())
    {
        process_user_input();

        auto& input = user_input[input_id_curr];
        
        idsp::update(input, io_state);
        render_input_display(io_state.display);

        render_camera(camera_state.display);

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


// Helper functions to use DirectX11
bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}


#include "../../../../libs/alloc_type/alloc_type.cpp"
#include "../../../../libs/image/image.cpp"
#include "../../../../libs/qsprintf/qsprintf.cpp"
#include "../../../../libs/sdl/sdl_input.cpp"
#include "../../../../libs/span/span.cpp"
#include "../../../../libs/stb_image/stb_image_options.hpp"

#include "../../../../libs/usb/camera_win.cpp"

#include "../../camera_display/camera_display.cpp"
#include "../../input_display/input_display.cpp"
#include "../../diagnostics/diagnostics.cpp"