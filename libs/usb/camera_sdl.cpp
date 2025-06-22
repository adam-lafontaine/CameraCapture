#include "camera_usb.hpp"

#include <SDL3/SDL.h>

#include "../image/convert.hpp"
#include "../util/numeric.hpp"


#ifndef NDEBUG
#define PRINT_MESSAGES
#endif

#ifdef PRINT_MESSAGES
#include <cstdio>
#endif


namespace sdl
{
    static void print_message(const char* msg)
    {
    #ifdef PRINT_MESSAGES
        printf("%s\n", msg);
    #endif
    }


    static void print_error(const char* msg)
    {
    #ifdef PRINT_MESSAGES
        printf("%s\n%s\n", msg, SDL_GetError());
    #endif
    }


    static u32 ns_to_fps(Uint64 ns)
    {
        constexpr f32 nano = (f32)SDL_NS_PER_SECOND;
        return num::round_to_unsigned<u32>(nano / ns);
    }
}


/* definitions */

namespace camera_usb
{
    namespace num = numeric;
    namespace img = image;
    namespace mb = memory_buffer;
    namespace cvt = convert;


    constexpr u32 DEVICE_COUNT_MAX = sizeof(CameraList::list) / sizeof(Camera);

    // TODO
    constexpr u32 FRAME_HEIGHT_PX = HEIGHT_MAX;
    constexpr u32 FRAME_WIDTH_PX = WIDTH_MAX;


    class DeviceSDL
    {
    public:
        SDL_CameraID device_id = 0;

        SDL_Camera* p_device = 0;
        SDL_CameraSpec spec;

        f32 grab_ns;

        img::ImageView rgba;
        img::View3u8 view3; // planar rgb or yuv        
    };


    class DeviceListSDL
    {
    public:

        DeviceSDL devices[DEVICE_COUNT_MAX] = { 0 };

        u32 count = 0;

        img::Buffer32 data32;
        img::Buffer8 data8;
    };


    
}


/* devices */

namespace camera_usb
{
    static SDL_CameraSpec format_yv12(DeviceSDL& device, u32 width, u32 height)
    {
        /*SDL_CameraSpec s{};
        s.format = SDL_PIXELFORMAT_RGBA32;
        s.colorspace = SDL_COLORSPACE_RGB_DEFAULT;
        s.width = 0;
        s.height = 0;

        int count = 0;
        auto formats = SDL_GetCameraSupportedFormats(device.device_id, &count);
        if (!formats)
        {
            return s;
        }        

        f32 max_fps = 0;
        int w = width;
        int h = height;
        for (int i = 0; i < count; i++)
        {
            auto sp = formats[i];
            if (sp->width == w && sp->height == h)
            {

            }

        }

        SDL_free(formats);

        return s;*/

        SDL_CameraSpec s{};

        s.colorspace = SDL_COLORSPACE_YUV_DEFAULT;
        s.format = SDL_PIXELFORMAT_YV12;
        s.width = width;
        s.height = height;
        s.framerate_numerator = 10'000'000;
        s.framerate_denominator = 10'000'000 / 30;

        return s;
    }


    static bool open_device(DeviceSDL& device)
    {
        if (!device.device_id)
        {
            sdl::print_message("No device ID");
            return false;
        }

        auto format = format_yv12(device, FRAME_WIDTH_PX, FRAME_HEIGHT_PX);

        auto camera = SDL_OpenCamera(device.device_id, &format);
        if (!camera)
        {
            sdl::print_error("SDL_OpenCamera()");
            return false;
        }

        /*if (!SDL_GetCameraFormat(camera, &device.spec))
        {
            return false;
        }*/

        device.p_device = camera;
        device.spec = format;

        return true;
    }


    static void close_device(DeviceSDL& device)
    {
        SDL_CloseCamera(device.p_device);
    }


    static bool grab_and_convert_frame_yuv(DeviceSDL& device, img::View3u8 const& dst)
    {
        Uint64 ts = 0;
        SDL_Surface* frame = 0;
        while (!frame)
        {
            frame = SDL_AcquireCameraFrame(device.p_device, &ts);
        }

        auto data = (u8*)frame->pixels;
        auto w = (u32)frame->w;
        auto h = (u32)frame->h;
        auto len = w * h + w * h / 2;

        auto span = span::make_view(data, len);

        auto format = cvt::PixelFormat::YV12;

        cvt::to_yuv(span, w, h, dst, format);

        SDL_ReleaseCameraFrame(device.p_device, frame);

        return true;
    }


    static bool grab_and_convert_frame_rgb(DeviceSDL& device, img::View3u8 const& dst)
    {
        Uint64 ts = 0;
        SDL_Surface* frame = 0;
        while (!frame)
        {
            frame = SDL_AcquireCameraFrame(device.p_device, &ts);
        }

        auto data = (u8*)frame->pixels;
        auto w = (u32)frame->w;
        auto h = (u32)frame->h;
        auto len = w * h + w * h / 2;

        auto span = span::make_view(data, len);

        auto format = cvt::PixelFormat::YV12;

        cvt::to_yuv(span, w, h, device.view3, format);
        cvt::yuv_to_rgb(device.view3, dst);

        SDL_ReleaseCameraFrame(device.p_device, frame);

        return true;
    }


    static bool grab_and_convert_frame_rgba(DeviceSDL& device, img::ImageView const& dst)
    {
        Uint64 ts = 0;
        SDL_Surface* frame = 0;
        while (!frame)
        {
            frame = SDL_AcquireCameraFrame(device.p_device, &ts);
        }

        auto data = (u8*)frame->pixels;
        auto w = (u32)frame->w;
        auto h = (u32)frame->h;
        auto len = w * h + w * h / 2;

        auto span = span::make_view(data, len);

        auto format = cvt::PixelFormat::YV12;

        cvt::to_yuv(span, w, h, device.view3, format);
        cvt::yuv_to_rgba(device.view3, dst);

        SDL_ReleaseCameraFrame(device.p_device, frame);

        return true;
    }
}


/* enumerate */

namespace camera_usb
{
    static bool enumerate_devices(DeviceListSDL& list)
    {
        int count = 0;
        SDL_CameraID* ids = SDL_GetCameras(&count);
        if (!ids || !count)
        {
            sdl::print_error("SDL_GetCameras()");
            return false;
        }

        list.count = 0;
        for (int i = 0; i < count; i++)
        {
            auto id = ids[i];
            printf("ID: %u\n", id);
            if (!id)
            {
                continue;
            }

            auto& device = list.devices[list.count];
            device.device_id = id;

            list.count++;
        }

        SDL_free(ids);

        return true;
    }


    static void close_devices(DeviceListSDL& list)
    {
        for (u32 i = 0; i < list.count; i++)
        {
            close_device(list.devices[i]);
        }
    }
}


/* static data */

namespace camera_usb
{
    DeviceListSDL sdl_devices;
}


namespace camera_usb
{
    CameraList enumerate_cameras()
    {
        CameraList cameras{};

        auto quit = [&]()
        {
            cameras.count = 0;
            cameras.status = ConnectionStatus::Disconnected;
            mb::destroy_buffer(sdl_devices.data32);
            mb::destroy_buffer(sdl_devices.data8);
        };

        cameras.status = ConnectionStatus::Connecting;

        if (!SDL_InitSubSystem(SDL_INIT_CAMERA))
        {
            sdl::print_error("SDL_InitSubSystem(SDL_INIT_CAMERA)");
            quit();
            return cameras;
        }

        if (!enumerate_devices(sdl_devices))
        {
            quit();
            return cameras;
        }

        auto n_pixels = WIDTH_MAX * HEIGHT_MAX;

        sdl_devices.data32 = img::create_buffer32(n_pixels, "data32");
        if (!sdl_devices.data32.ok)
        {
            quit();
            return cameras;
        }

        sdl_devices.data8 = img::create_buffer8(3 * n_pixels, "data8");
        if (!sdl_devices.data8.ok)
        {
            quit();
            return cameras;
        }

        cameras.count = num::min(DEVICE_COUNT_MAX, sdl_devices.count);
        for (u32 i = 0; i < cameras.count; i++)
        {
            auto& camera = cameras.list[i];
            auto& device = sdl_devices.devices[i];

            camera.id = i;
            camera.status = CameraStatus::Active;

            // TODO
            camera.vendor = span::to_string_view("XXXX");
            camera.product = span::to_string_view("XXXX");
            camera.serial_number = span::to_string_view("XXXX");
            camera.label = span::to_string_view("XXXX");

            camera.format = span::to_string_view("XXXX");
        }

        cameras.status = ConnectionStatus::Connected;

        return cameras;
    }


    void close(CameraList& cameras)
    {
        mb::destroy_buffer(sdl_devices.data32);
        mb::destroy_buffer(sdl_devices.data8);

        close_devices(sdl_devices);

        for (u32 i = 0; i < cameras.count; i++)
        {
            auto& camera = cameras.list[i];
            camera.id = -1;
            camera.status = CameraStatus::Inactive;
        }

        SDL_QuitSubSystem(SDL_INIT_CAMERA);
    }


    bool open_camera(Camera& camera)
    {
        camera.busy = 1;
        auto& device = sdl_devices.devices[camera.id];

        if (!open_device(device))
        {
            return false;
        }

        camera.frame_width = device.spec.width;
        camera.frame_height = device.spec.height;

        auto n = device.spec.framerate_numerator;
        auto d = device.spec.framerate_denominator;
        camera.fps = d ? n / d : 0;

        auto& buffer32 = sdl_devices.data32;
        auto& buffer8 = sdl_devices.data8;
        auto w = camera.frame_width;
        auto h = camera.frame_height;
        mb::reset_buffer(buffer32);
        mb::reset_buffer(buffer8);
        device.rgba = img::make_view(w, h, buffer32);
        device.view3 = img::make_view_3(w, h, buffer8);

        if (!buffer32.ok || !buffer8.ok)
        {
            camera.busy = 0;
            return false;
        }

        camera.status = CameraStatus::Open;
        camera.busy = 0;

        return true;
    }


    void grab_image(Camera& camera, img::ImageView const& dst)
    {
        camera.busy = 1;
        auto& device = sdl_devices.devices[camera.id];

        auto start = SDL_GetTicksNS();

        if (!grab_and_convert_frame_rgba(device, dst))
        {
            img::fill(dst, img::to_pixel(0, 0, 255));
        }

        auto end = SDL_GetTicksNS();

        device.grab_ns = end - start;
        camera.fps = sdl::ns_to_fps(device.grab_ns);

        camera.busy = 0;
    }
    

    void stream_camera(Camera& camera, img::ImageView const& dst, bool_fn const& stream_condition)
    {
        camera.busy = 1;
        auto& device = sdl_devices.devices[camera.id];

        auto c_status = camera.status;

        camera.status = CameraStatus::Streaming;

        while (stream_condition())
        {
            auto start = SDL_GetTicksNS();

            if (!grab_and_convert_frame_rgba(device, dst))
            {
                img::fill(dst, img::to_pixel(0, 0, 255));
            }

            auto end = SDL_GetTicksNS();

            device.grab_ns = end - start;
            camera.fps = sdl::ns_to_fps(device.grab_ns);
        }

        camera.busy = 0;
        camera.status = c_status;
    }


    void stream_camera(Camera& camera, grab_cb const& on_grab, bool_fn const& stream_condition)
    {
        camera.busy = 1;
        auto& device = sdl_devices.devices[camera.id];

        auto c_status = camera.status;

        camera.status = CameraStatus::Streaming;

        while (stream_condition())
        {
            auto start = SDL_GetTicksNS();

            if (grab_and_convert_frame_rgba(device, device.rgba))
            {
                on_grab(device.rgba);
            }

            auto end = SDL_GetTicksNS();

            device.grab_ns = end - start;
            camera.fps = sdl::ns_to_fps(device.grab_ns);
        }

        camera.busy = 0;
        camera.status = c_status;
    }


    void grab_planar_rgb(Camera& camera, img::View3u8 const& dst)
    {
        camera.busy = 1;
        auto& device = sdl_devices.devices[camera.id];
        
        auto start = SDL_GetTicksNS();

        if (!grab_and_convert_frame_rgb(device, dst))
        {
            
        }

        auto end = SDL_GetTicksNS();

        device.grab_ns = end - start;
        camera.fps = sdl::ns_to_fps(device.grab_ns);

        camera.busy = 0;
    }


    void grab_planar_yuv(Camera& camera, img::View3u8 const& dst)
    {
        camera.busy = 1;
        auto& device = sdl_devices.devices[camera.id];
        
        auto start = SDL_GetTicksNS();

        if (!grab_and_convert_frame_yuv(device, dst))
        {
            
        }

        auto end = SDL_GetTicksNS();

        device.grab_ns = end - start;
        camera.fps = sdl::ns_to_fps(device.grab_ns);

        camera.busy = 0;
    }

    
    void stream_planar_rgb(Camera& camera, planar_cb const& proc, bool_fn const& stream_condition)
    {
        camera.busy = 1;
        auto& device = sdl_devices.devices[camera.id];

        auto c_status = camera.status;

        camera.status = CameraStatus::Streaming;

        while (stream_condition())
        {
            auto start = SDL_GetTicksNS();

            if (grab_and_convert_frame_rgb(device, device.view3))
            {
                proc(device.view3);
            }

            auto end = SDL_GetTicksNS();

            device.grab_ns = end - start;
            camera.fps = sdl::ns_to_fps(device.grab_ns);
        }

        camera.busy = 0;
        camera.status = c_status;
    }


    void stream_planar_yuv(Camera& camera, planar_cb const& proc, bool_fn const& stream_condition)
    {
        camera.busy = 1;
        auto& device = sdl_devices.devices[camera.id];

        auto c_status = camera.status;

        camera.status = CameraStatus::Streaming;

        while (stream_condition())
        {
            auto start = SDL_GetTicksNS();

            if (grab_and_convert_frame_yuv(device, device.view3))
            {
                proc(device.view3);
            }

            auto end = SDL_GetTicksNS();

            device.grab_ns = end - start;
            camera.fps = sdl::ns_to_fps(device.grab_ns);
        }

        camera.busy = 0;
        camera.status = c_status;
    }
}