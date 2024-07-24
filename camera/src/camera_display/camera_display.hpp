#pragma once

#include "../../../libs/usb/camera_usb.hpp"


namespace cam = camera_usb;


namespace camera_display
{
    class CameraState
    {
    public:
        cam::CameraList cameras;

    };


    bool init(CameraState& state);

    void show_cameras(CameraState& state);
}