#include "../imgui_sdl2_dx11/imgui_include.hpp"
#include "../../camera_display/camera_display.hpp"

#ifndef NDEBUG
#include "../../diagnostics/diagnostics.hpp"
#endif


namespace img = image;
namespace cdsp = camera_display;


static void set_window_icon(SDL_Window* window)
{
#include "../../../../resources/icon_64.c" // this will "paste" the struct my_icon into this function
    ui_imgui::set_window_icon(window, icon_64);
}


static void texture_window(cstr title, ImTextureID texture, u32 width, u32 height, f32 scale)
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
    dx11_imgui::TextureList<N_TEXTURES> textures;
    
    cdsp::CameraState camera_state{};
    img::Buffer32 camera_buffer;
    
    constexpr dx11_imgui::TextureId camera_texture_id = dx11_imgui::to_texture_id(0);
}


static void end_program()
{
    mv::run_state = RunState::End;
}


static bool is_running()
{
    return mv::run_state != RunState::End;
}


static void init_camera_display()
{
    // TODO init camera app
    u32 w = 640;
    u32 h = 480;
    mv::camera_buffer = img::create_buffer32(w * h, "camera display");
    mv::camera_state.display = img::make_view(w, h, mv::camera_buffer);
    img::fill(mv::camera_state.display, img::to_pixel(128));

    cdsp::init_async(mv::camera_state);

    auto data = mv::camera_state.display.matrix_data_;
    auto& t = mv::textures.get_dx_texture_ref(mv::camera_texture_id);

    dx11_imgui::init_texture(data, w, h, t, mv::ui_state.dx_context);
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
    mv::ui_state.window_title = "USB Camera";

    // fullscreen?
    mv::ui_state.window_width = 1200;
    mv::ui_state.window_height = 800;

    if (!ui_imgui::init(mv::ui_state))
    {
        return false;
    }

    set_window_icon(mv::ui_state.window);
    mv::textures = dx11_imgui::create_textures<mv::N_TEXTURES>();

    init_camera_display();

    return true;
}


static void main_close()
{
    cdsp::close_async(mv::camera_state);
    ui_imgui::close(mv::ui_state);
    mb::destroy_buffer(mv::camera_buffer);
}


static void main_loop()
{
    auto& camera_texture = mv::textures.get_dx_texture_ref(mv::camera_texture_id);
    auto& ctx = mv::ui_state.dx_context;

    while(is_running())
    {
        ui_imgui::handle_sdl_events(mv::ui_state);          
        dx11_imgui::render_texture(camera_texture, ctx);

        render_imgui_frame();

        if (mv::ui_state.cmd_end_program)
        {
            end_program();
        }
    }
}


int main()
{
    if (!main_init())
    {
        return EXIT_FAILURE;
    }

    mv::run_state = RunState::Run;

    main_loop();

    return EXIT_SUCCESS;
}

#include "main_o.cpp"