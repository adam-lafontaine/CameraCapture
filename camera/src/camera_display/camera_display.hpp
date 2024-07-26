#pragma once

#include "../../../libs/usb/camera_usb.hpp"


namespace cam = camera_usb;


namespace camera_display
{
    


    class CameraState
    {
    public:
        cam::CameraList cameras;

        bool is_connected() const { return cameras.status == cam::ConnectionStatus::Connected; }        

    };


    void init_async(CameraState& state);

    void close(CameraState& state);

    void show_cameras(CameraState& state);
}