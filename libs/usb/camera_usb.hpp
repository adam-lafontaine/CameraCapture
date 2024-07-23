#pragma once

#include "../image/image.hpp"


namespace camera_usb
{
    class Camera
    {
    public:
        int device_id = -1;
		u32 frame_width = 0;
		u32 frame_height = 0;
		u32 max_fps = 0;

		bool is_open = false;
    };
}