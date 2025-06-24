#pragma once

#include "imgui_options.hpp"

// v1.91.5
#include "../../../../libs/imgui_1_91/backends/imgui_impl_sdl2.h"
#include "../../../../libs/imgui_1_91/backends/imgui_impl_dx11.h"

#include <d3d11.h>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>


/* dx11 context */

namespace ui_imgui
{
namespace dx
{
    class Context
    {
    public:
        ID3D11Device*            pd3dDevice = nullptr;
        ID3D11DeviceContext*     pd3dDeviceContext = nullptr;
        IDXGISwapChain*          pSwapChain = nullptr;
        ID3D11RenderTargetView*  mainRenderTargetView = nullptr;
    };


    static void create_render_target(Context& ctx)
    {
        ID3D11Texture2D* pBackBuffer;
        ctx.pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
        ctx.pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &ctx.mainRenderTargetView);
        pBackBuffer->Release();
    }


    static void cleanup_render_target(Context& ctx)
    {
        if (ctx.mainRenderTargetView) { ctx.mainRenderTargetView->Release(); ctx.mainRenderTargetView = nullptr; }
    }


    static bool init_context(Context& ctx, HWND hWnd)
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

        auto res = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 
            createDeviceFlags, featureLevelArray, 2, 
            D3D11_SDK_VERSION, &sd, 
            &ctx.pSwapChain, 
            &ctx.pd3dDevice, 
            &featureLevel, 
            &ctx.pd3dDeviceContext
            );
        
        if (res != S_OK)
        {
            return false;
        }            

        create_render_target(ctx);
        return true;
    }


    static void close_context(Context& ctx)
    {
        auto const release = [](auto& p) 
        {
            if (p)
            {
                p->Release();
                p = nullptr;
            }
        };

        release(ctx.mainRenderTargetView);
        release(ctx.pSwapChain);
        release(ctx.pd3dDeviceContext);
        release(ctx.pd3dDevice);
    }


} // dx
}


namespace ui_imgui
{
    class UIState
    {
    public:

        const char* window_title = 0;
        int window_width = 0;
        int window_height = 0;

        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

        SDL_Window* window = 0;
        dx::Context dx_context = {};
        HWND hwnd = 0;

        bool is_fullscreen = false;
        bool cmd_end_program = false;

#ifdef SHOW_IMGUI_DEMO

        bool show_demo_window = true;
        bool show_another_window = false;

#endif
    };


    bool init(UIState& state)
    {
        auto sdl_flags = SDL_INIT_VIDEO;
        sdl_flags |= SDL_INIT_TIMER;
        //sdl_flags |= SDL_INIT_GAMECONTROLLER;

        if (SDL_Init(sdl_flags) != 0)
        {
            return false;
        }

        SDL_WindowFlags window_flags = (SDL_WindowFlags)(
            SDL_WINDOW_OPENGL | 
            SDL_WINDOW_RESIZABLE | 
            SDL_WINDOW_ALLOW_HIGHDPI
        );

        /*if (!state.window_width || !state.window_height)
        {
            window_flags |= SDL_WINDOW_FULLSCREEN;
            state.is_fullscreen = true;
        }*/
            
        SDL_Window* window = 
            SDL_CreateWindow(
                state.window_title, 
                SDL_WINDOWPOS_CENTERED, 
                SDL_WINDOWPOS_CENTERED, 
                state.window_width, 
                state.window_height, 
                window_flags);
        
        if (!window)
        {
            return false;
        }

        SDL_SysWMinfo wmInfo;
        SDL_VERSION(&wmInfo.version);
        SDL_GetWindowWMInfo(window, &wmInfo);
        HWND hwnd = (HWND)wmInfo.info.win.window;

        // Initialize Direct3D
        dx::Context ctx{};
        if (!dx::init_context(ctx, hwnd))
        {
            dx::close_context(ctx);
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
        ImGui_ImplDX11_Init(ctx.pd3dDevice, ctx.pd3dDeviceContext);

        // Load Fonts
        // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
        // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
        // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
        // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
        // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
        // - Read 'docs/FONTS.md' for more instructions and details.
        // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
        // - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.
        //io.Fonts->AddFontDefault();
        //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
        //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
        //IM_ASSERT(font != nullptr);

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsLight();

        constexpr auto TEXT_WHITE = ImVec4(0.7f, 0.7f, 0.7f, 1);

        auto& style = ImGui::GetStyle();
        style.Colors[ImGuiCol_Text] = TEXT_WHITE;
        style.TabRounding = 0.0f;

        state.window = window;
        state.hwnd = hwnd;
        state.dx_context = ctx;

        return true;
    }


    void new_frame()
    {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // Rendering
        ImGui::DockSpaceOverViewport();
        //ImGui::DockSpaceOverViewport(nullptr, ImGuiDockNodeFlags_None);
    }
    
    
    void render(UIState& state)
    {
        auto& io = ImGui::GetIO();
        auto& clear_color = state.clear_color;
        auto& ctx = state.dx_context;

        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        ImGui::Render();
        ctx.pd3dDeviceContext->OMSetRenderTargets(1, &ctx.mainRenderTargetView, nullptr);
        ctx.pd3dDeviceContext->ClearRenderTargetView(ctx.mainRenderTargetView, clear_color_with_alpha);

        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        ctx.pSwapChain->Present(1, 0); // Present with vsync
        //ctx.pSwapChain->Present(0, 0); // Present without vsync
    }


    void close(UIState& state)
    {
        // Cleanup
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();

        dx::close_context(state.dx_context);

        SDL_DestroyWindow(state.window);
        SDL_Quit();
    }}


/* sdl wrappers */

namespace ui_imgui
{
    inline void set_window_icon(SDL_Window* window, auto const& icon_64)
    {
        // these masks are needed to tell SDL_CreateRGBSurface(From)
        // to assume the data it gets is byte-wise RGB(A) data
        Uint32 rmask, gmask, bmask, amask;
    #if SDL_BYTEORDER == SDL_BIG_ENDIAN
        int shift = (icon_64.bytes_per_pixel == 3) ? 8 : 0;
        rmask = 0xff000000 >> shift;
        gmask = 0x00ff0000 >> shift;
        bmask = 0x0000ff00 >> shift;
        amask = 0x000000ff >> shift;
    #else // little endian, like x86
        rmask = 0x000000ff;
        gmask = 0x0000ff00;
        bmask = 0x00ff0000;
        amask = (icon_64.bytes_per_pixel == 3) ? 0 : 0xff000000;
    #endif

        SDL_Surface* icon = SDL_CreateRGBSurfaceFrom(
            (void*)icon_64.pixel_data,
            icon_64.width,
            icon_64.height,
            icon_64.bytes_per_pixel * 8,
            icon_64.bytes_per_pixel * icon_64.width,
            rmask, gmask, bmask, amask);

        SDL_SetWindowIcon(window, icon);

        SDL_FreeSurface(icon);
    }


    inline void handle_sdl_events(UIState& state)
    {  
        SDL_Keycode key_code;

        SDL_Event event;
        auto const window_resize = [&]()
        {
            if (event.window.windowID != SDL_GetWindowID(state.window))
            {
                return;
            }

            auto& ctx = state.dx_context;
            
            int w, h;
            SDL_GetWindowSize(state.window, &w, &h);

            // Release all outstanding references to the swap chain's buffers before resizing.
            dx::cleanup_render_target(ctx);
            ctx.pSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
            dx::create_render_target(ctx);
        };

        while (SDL_PollEvent(&event))
        {
            auto window = SDL_GetWindowFromID(event.window.windowID);

            switch (event.type)
            {
            case SDL_QUIT:
            {
                state.cmd_end_program = true;
            } break;

            case SDL_KEYDOWN:
            {
                key_code = event.key.keysym.sym;
                auto alt = event.key.keysym.mod & KMOD_ALT;

                switch (key_code)
                {
                case SDLK_F4:
                {
                    if (alt)
                    {
                        state.cmd_end_program = true;
                    }                    
                } break;

                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                {
                    if (alt)
                    {
                        // TODO: broken
                        //SDL_WINDOW_FULLSCREEN
                        //auto flags = state.is_fullscreen ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP;
                        //SDL_SetWindowFullscreen(window, flags);
                        //state.is_fullscreen = !state.is_fullscreen;
                        assert(false && "TODO: toggle fullscreen");
                    }
                    
                } break;

                #ifndef NDEBUG
                case SDLK_ESCAPE:
                {
                    state.cmd_end_program = true;
                } break;
                #endif

                default:
                    break;
                }
            } break;

            case SDL_WINDOWEVENT:
            {
                switch (event.window.event)
                {
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                case SDL_WINDOWEVENT_RESIZED:
                    window_resize();
                    break;

                case SDL_WINDOWEVENT_CLOSE:
                {
                    state.cmd_end_program = true;
                } break;
                
                default:
                    break;
                }
            } break;

            default:
                break;
            }

            ImGui_ImplSDL2_ProcessEvent(&event);
        }
    }

}


/* demo window */

namespace ui_imgui
{
    static inline void show_imgui_demo(UIState& state)
    {
    #ifdef SHOW_IMGUI_DEMO

        auto& io = ImGui::GetIO();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (state.show_demo_window)
            ImGui::ShowDemoWindow(&state.show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &state.show_demo_window);      // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &state.show_another_window);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&state.clear_color); // Edit 3 floats representing a color

            if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }

        // 3. Show another simple window.
        if (state.show_another_window)
        {
            ImGui::Begin("Another Window", &state.show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                state.show_another_window = false;
            ImGui::End();
        }

    #endif
    }

}


/* dx11 texture */

namespace dx11_imgui
{
    // https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples


    using Context = ui_imgui::dx::Context;


    struct TextureId { int value = -1; };


    constexpr TextureId to_texture_id(int value) { return { value }; };


    class Texture
    {
    public:
        ID3D11Texture2D* pTexture = 0;
        ID3D11ShaderResourceView* srv;
        TextureId id;

        int image_width;
        int image_height;
        void* image_data;
    };


    template <size_t N>
    class TextureList
    {
    public:
        static constexpr size_t count = N;

        Texture data[count] = { 0 };

        Texture& get_dx_texture_ref(TextureId id) { return data[id.value]; }

        ImTextureID get_im_texture_id(TextureId id) { return (ImTextureID)(intptr_t)data[id.value].srv; }
    };


    template <size_t N>
    static inline TextureList<N> create_textures()
    {
        TextureList<N> textures{};

        for (int i = 0; i < textures.count; i++)
        {
            auto& texture = textures.data[i];
            texture.id.value = i;
        }

        return textures;
    }


    template <typename P>
    static inline void init_texture(P* data, int width, int height, Texture& texture, Context& ctx)
    {
        static_assert(sizeof(P) == 4);

        texture.image_data = (void*)data;
        texture.image_width = width;
        texture.image_height = height;
        
        auto& pTexture = texture.pTexture;
        auto& srv = texture.srv;

        D3D11_TEXTURE2D_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;        

        D3D11_SUBRESOURCE_DATA subResource;
        subResource.pSysMem = (void*)data;
        subResource.SysMemPitch = desc.Width * 4;
        subResource.SysMemSlicePitch = 0;
        ctx.pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        ZeroMemory(&srvDesc, sizeof(srvDesc));
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = desc.MipLevels;
        srvDesc.Texture2D.MostDetailedMip = 0;
        ctx.pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, &srv);

        ctx.pd3dDeviceContext->PSSetShaderResources(0, 1, &srv);
    }
    

    static inline void render_texture(Texture& texture, Context& ctx)
    {
        ctx.pd3dDeviceContext->UpdateSubresource(
            texture.pTexture, 
            0, 0, 
            texture.image_data,
            texture.image_width * 4, 
            0);

        ctx.pd3dDeviceContext->PSSetShaderResources(0, 1, &texture.srv);
    }
}