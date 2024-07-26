#pragma once

#include "../image/image.hpp"


namespace camera_usb
{
    enum class ConnectionStatus : u8
    {
        Disconnected,
        Connecting,
        Connected
    };


    enum class CameraStatus : u8
    {
        Inactive,
        Active,
    };


    class Camera
    {
    public:
        int id = -1;

		u32 frame_width = 0;
		u32 frame_height = 0;
		u32 fps = 0;

        StringView format;

        StringView vendor;
        StringView product;
        StringView serial_number;

        StringView label;

        CameraStatus status = CameraStatus::Inactive;
    };


    class CameraList
    {
    public:
        Camera list[16];

        u32 count = 0;

        ConnectionStatus status = ConnectionStatus::Disconnected;
    };


    CameraList enumerate_cameras();

    void close(CameraList& cameras);
}