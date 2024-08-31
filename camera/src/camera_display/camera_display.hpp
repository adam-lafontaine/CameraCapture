#pragma once

#include "../../../libs/usb/camera_usb.hpp"


namespace cam = camera_usb;
namespace img = image;


namespace camera_display
{
    class CameraState
    {
    public:

        img::ImageView display;
        f32 histogram[64] = { 0 };

        cam::CameraList cameras; 

        bool is_streaming = false;


        static constexpr auto hist_count = sizeof(histogram) / sizeof(histogram[0]);
    };


    void init_async(CameraState& state);

    void close_async(CameraState& state);

    void show_cameras(CameraState& state);
}