#pragma once

#include "../../../libs/input/input.hpp"
#include "../../../libs/image/image.hpp"


namespace app
{
    class StateData;


    class AppState
    {
    public:
        image::ImageView screen;

        StateData* data_ = nullptr;        
    };


    class AppResult
    {
    public:
        bool success = false;

        Vec2Du32 screen_dimensions;
    };


    AppResult init(AppState& state);

    bool set_screen_memory(AppState& state, image::ImageView screen);

    void update(AppState& state, input::Input const& input);

    void close(AppState& state);
}