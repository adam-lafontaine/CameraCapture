#include "../../imgui_sdl_ogl/imgui_include.hpp"
#include "../../../../libs/sdl/sdl.cpp"

namespace img = image;


/* opengl */

namespace ogl
{
    union GLTextures
    {
        static constexpr int id_camera = 0;

        static constexpr int count = 1; 

        GLuint list[count] = { 0 };

        struct
        {
            GLuint camera_texture;
        };
        
    };


    

}




SDL_Window* create_sdl_window()
{
    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    
    auto window_width = ENGINE_WINDOW_WIDTH;
    auto window_height = ENGIN_WINDOW_HEIGHT;


    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = 
        SDL_CreateWindow(
            ENGINE_WINDOW_TITLE, 
            SDL_WINDOWPOS_CENTERED, 
            SDL_WINDOWPOS_CENTERED, 
            window_width, 
            window_height, 
            window_flags);

    return window;
}


static void set_game_window_icon(SDL_Window* window)
{
//#include "../../../resources/icon_64.c" // this will "paste" the struct my_icon into this function
    //sdl::set_window_icon(window, icon_64);
}