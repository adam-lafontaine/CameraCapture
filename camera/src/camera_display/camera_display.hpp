#pragma once

#include "../../../libs/usb/camera_usb.hpp"


namespace cam = camera_usb;


namespace camera_display
{
    enum class ConnectionStatus : u8
    {
        Disconnected,
        Connecting,
        Connected
    };


    class CameraState
    {
    public:
        cam::CameraList cameras;

        ConnectionStatus connection = ConnectionStatus::Disconnected;

    };


    void init_async(CameraState& state);

    void close(CameraState& state);

    void show_cameras(CameraState& state);
}