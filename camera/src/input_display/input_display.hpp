#pragma once

#include "../../../libs/input/input.hpp"
#include "../../../libs/image/image.hpp"



namespace input_display
{
    class StateData;


    class IOState
    {
    public:
        image::ImageView display;

        StateData* data_ = nullptr;
    };


    bool init(IOState& state);

    void update(input::Input const& input, IOState& state);

    void close(IOState& state);
}
