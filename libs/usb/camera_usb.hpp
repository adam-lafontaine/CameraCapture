#pragma once

#include "../image/image.hpp"

namespace img = image;


/* status enum */

namespace camera_usb
{
    enum class ConnectionStatus : u8
    {
        Disconnected = 0,
        Connecting,
        Connected
    };
    

    enum class CameraStatus : u8
    {
        Inactive = 0,
        Active,
        Open,
        Streaming
    };
}


namespace camera_usb
{
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
        b8 busy = 0;
    };


    class CameraList
    {
    public:
        Camera list[16];

        u32 count = 0;

        ConnectionStatus status = ConnectionStatus::Disconnected;
    };


    using bool_fn = std::function<bool()>;
    using grab_cb = std::function<void(image::ImageView const&)>;


    CameraList enumerate_cameras();

    void close(CameraList& cameras);


    void close_camera(Camera& camera);

    bool open_camera(Camera& camera);

    void stream_camera(Camera& camera, grab_cb const& on_grab, bool_fn const& stream_condition);
}