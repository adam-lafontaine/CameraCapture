#pragma once

#include "../image/image.hpp"


/* constants */

namespace camera_usb
{
    constexpr u32 WIDTH_MAX = 640;
    constexpr u32 HEIGHT_MAX = 480;

    namespace img = image;
}


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

        bool is_open() const { return status >= CameraStatus::Open; }
    };


    class CameraList
    {
    public:
        Camera list[16];

        u32 count = 0;

        ConnectionStatus status = ConnectionStatus::Disconnected;
    };


    using bool_fn = std::function<bool()>;
    using grab_cb = std::function<void(img::ImageView const&)>;
    using planar_cb = std::function<void(img::View3u8 const&)>;


    CameraList enumerate_cameras();

    void close(CameraList& cameras);

    bool open_camera(Camera& camera);

    void grab_image(Camera& camera, img::ImageView const& dst);
    

    void stream_camera(Camera& camera, img::ImageView const& dst, bool_fn const& stream_condition);

    void stream_camera(Camera& camera, grab_cb const& on_grab, bool_fn const& stream_condition);


    void grab_planar_rgb(Camera& camera, img::View3u8 const& dst);

    void grab_planar_yuv(Camera& camera, img::View3u8 const& dst);

    
    void stream_planar_rgb(Camera& camera, planar_cb const& proc, bool_fn const& stream_condition);

    void stream_planar_yuv(Camera& camera, planar_cb const& proc, bool_fn const& stream_condition);
}