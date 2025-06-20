#include "../imgui_sdl3_ogl3/imgui_include.hpp"
#include "../../camera_display/camera_display.hpp"

namespace img = image;
namespace cdsp = camera_display;


static void set_window_icon(SDL_Window* window)
{
#include "../../../../resources/icon_64.c" // this will "paste" the struct my_icon into this function
    ui_imgui::set_window_icon(window, icon_64);
}


static void texture_window(cstr title, ImTextureID image_texture, u32 width, u32 height, f32 scale)
{
    auto w = width * scale;
    auto h = height * scale;

    ImGui::Begin(title);

    ImGui::Image(image_texture, ImVec2(w, h));

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
    ogl_imgui::TextureList<N_TEXTURES> textures;

    cdsp::CameraState camera_state{};
    img::Buffer32 camera_buffer;
    
    constexpr ogl_imgui::TextureId camera_texture_id = ogl_imgui::to_texture_id(0);
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
    auto t = mv::textures.get_ogl_texture(mv::camera_texture_id);

    ogl_imgui::init_texture(data, w, h, t);
}


static void render_imgui_frame()
{
    ui_imgui::handle_sdl_events(mv::ui_state);
    ui_imgui::new_frame();
    ui_imgui::show_imgui_demo(mv::ui_state);

    /*auto t = textures.get_imgui_texture(camera_texture_id);
    auto w = camera_state.display.width;
    auto h = camera_state.display.height;
    auto scale = 1.0f;
    texture_window("Camera", t, w, h, scale);*/


    ui_imgui::render(mv::ui_state);

    if (mv::ui_state.cmd_end_program)
    {
        end_program();
    }
}


static bool main_init()
{
    mv::ui_state.window_title = "USB Camera";

    // fullscreen
    mv::ui_state.window_width = 1000;
    mv::ui_state.window_height = 800;

    if (!ui_imgui::init(mv::ui_state))
    {
        return false;
    }

    set_window_icon(mv::ui_state.window);
    mv::textures = ogl_imgui::create_textures<mv::N_TEXTURES>();

    //init_camera_display();

    return true;
}


static void main_close()
{
    //cdsp::close_async(mv::camera_state);
    ui_imgui::close(mv::ui_state);
    mb::destroy_buffer(mv::camera_buffer);
}


static void main_loop()
{
    auto camera_texture = mv::textures.get_ogl_texture(mv::camera_texture_id);

    while(is_running())
    {
        //ogl_imgui::render_texture(camera_texture);
        render_imgui_frame();

        
    }
}


int main()
{
    if (!main_init())
    {
        return EXIT_FAILURE;
    }

    mv::run_state = RunState::Run;

    while (is_running())
    {
        main_loop();
    }

    main_close();

    return EXIT_SUCCESS;
}


#include "main_o.cpp"