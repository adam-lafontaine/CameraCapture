#pragma once

#include "../image/image.hpp"


namespace camera_usb
{
    class Camera
    {
    public:
        int id = -1;

		u32 frame_width = 0;
		u32 frame_height = 0;
		u32 fps = 0;

        cstr format;
    };


    class CameraList
    {
    public:
        Camera list[16];

        u32 count = 0;
    };


    CameraList enumerate_cameras();
}