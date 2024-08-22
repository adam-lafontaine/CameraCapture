#pragma once

#include "camera_usb.hpp"
#include "libuvc3.hpp"
#include "../image/convert.hpp"
#include "../qsprintf/qsprintf.hpp"
#include "../util/numeric.hpp"
#include "../util/stopwatch.hpp"

#include <cassert>

namespace num = numeric;


namespace camera_usb
{
    namespace img = image;
    namespace cvt = convert;

    constexpr u8 DEVICE_COUNT_MAX = 16;


    class DeviceConfigUVC
    {
    public:
        u32 frame_width = 0;
        u32 frame_height = 0;
        u32 fps = 0;

        //uvc::uvc_frame_format uvc_format;

        cvt::PixelFormat pixel_format = cvt::PixelFormat::Unknown;
        char format_code[5] = { 0 };
    };


    class DeviceUVC
    {
    public:
        int device_id = -1;

        uvc::device* p_device = nullptr;
        uvc::device_handle* h_device = nullptr;
        uvc::stream_ctrl ctrl;
        uvc::stream_handle* h_stream = nullptr;
        //uvc::frame_desc* p_frame_desc = nullptr;

        char product_id[5] = { 0 };
        char vendor_id[5] = { 0 };
        char serial_number[32] = { 0 };

        char label[32] = { 0 };
        
        DeviceConfigUVC config;

        Stopwatch grab_sw;
        f32 grab_ms;

        img::ImageView rgba;
        convert::ViewYUV yuv;
    };


    class DeviceListUVC
    {
    public:
        uvc::context* context = nullptr;
        uvc::device** device_list = nullptr;

        DeviceUVC devices[DEVICE_COUNT_MAX] = { 0 };

        u32 count = 0;

        img::Buffer32 data32;
    };
}


namespace camera_usb
{
/*

libuvc requires RW permissions for opening capturing devices, so you must
create the following .rules file:

/etc/udev/rules.d/99-uvc.rules

Then, for each webcam add the following line:

SUBSYSTEMS=="usb", ENV{DEVTYPE}=="usb_device", ATTRS{idVendor}=="XXXX", ATTRS{idProduct}=="YYYY", MODE="0666"

Replace XXXX and YYYY for the 4 hexadecimal characters corresponding to the
vendor and product ID of your webcams.

*/

    static void print_device_permissions_msg(DeviceListUVC const& list)
    {
#ifndef NDEBUG

    printf(
        "\n********** LINUX PERMISSIONS MESSAGE **********\n\n"
        "Libuvc requires RW permissions for opening capturing devices, so you must create the following .rules file:\n\n"
        "/etc/udev/rules.d/99-uvc.rules\n\n"
        "Add the following line(s) to register each device:\n\n"
        );

    auto const fmt = "SUBSYSTEMS==\"usb\", ENV{DEVTYPE}==\"usb_device\", ATTRS{idVendor}==\"%s\", ATTRS{idProduct}==\"%s\", MODE=\"0666\"\n";

    for (u32 i = 0; i < list.count; i++)
    {
        auto& cam = list.devices[i];
        printf(fmt, cam.vendor_id, cam.product_id);
    }

    printf(
        "\nRestart the computer for the changes to take effect."
        "\n\n********** LINUX PERMISSIONS MESSAGE **********\n\n"
        );

#endif
    }
}


/* wrappers */

namespace camera_usb
{
    static bool open_device(DeviceUVC& device)
    {
        assert(device.p_device && "No device to open");

        auto res = uvc::uvc_open(device.p_device, &device.h_device);
        
        return res == uvc::UVC_SUCCESS;
    }
    
    
    static void close_device(DeviceUVC& device)
    {
        if (device.h_device)
        {
            uvc::uvc_close(device.h_device);
            device.h_device = nullptr;
        } 
    }


    static uvc::opt::FrameFormat get_frame_format(DeviceUVC const& device)
    {
        using PF = cvt::PixelFormat;

        constexpr u32 N = 9;

        PF formats[N] {
            PF::YUYV,
            PF::YUNV,
            PF::YUY2,
            PF::YVYU,
            PF::UYVY,
            PF::Y422,
            PF::UYNV,
            PF::HDYC,
            PF::NV12
        };

        for (u32 i = 0; i < N; i++)
        {
            auto format = uvc::opt::find_frame_format(device.h_device, (u32)formats[i], 30);
            if (format.ok)
            {
                return format;
            }
        }

        uvc::opt::FrameFormat ff;

        return ff;        
    }
    
    
    static bool load_device_config(DeviceUVC& device)
    {
        if (!device.h_device)
        {
            assert(false && "Device not open");
            return false;
        }

        // test to see if config exists
        auto res = uvc::opt::uvc_get_stream_ctrl_format_size(
            device.h_device, 
            &device.ctrl, // result stored in ctrl
            (u32)device.config.pixel_format,
            device.config.frame_width, 
            device.config.frame_height, 
            device.config.fps);

        return res >= 0;
    }


    static void close_stream(DeviceUVC& device)
    {
        if (device.h_stream)
        {
            uvc::uvc_stop_streaming(device.h_device);
            device.h_stream = nullptr;
        }
    }


    static bool open_stream(DeviceUVC& device)
    {
        if (!device.h_device)
        {
            assert(false && "Device not open");
            return false;
        }

        auto res = uvc::uvc_stream_open_ctrl(device.h_device, &device.h_stream, &device.ctrl);
        
        return res == uvc::UVC_SUCCESS;
    }


    static bool start_stream_single_frame(DeviceUVC& device)
    {
        if (!device.h_stream)
        {
            assert(false && "Stream not open");
        }

        auto res = uvc::uvc_stream_start(device.h_stream, 0);

        return res == uvc::UVC_SUCCESS;
    }


    static void enable_exposure_mode(DeviceUVC const& device)
    {
        constexpr u8 EXPOSURE_MODE_MANUAL = 1;
        constexpr u8 EXPOSURE_MODE_AUTO = 2;
        constexpr u8 EXPOSURE_MODE_SHUTTER_PRIORITY = 4;
        constexpr u8 EXPOSURE_MODE_APERTURE = 8;

        auto res = uvc::uvc_set_ae_mode(device.h_device, EXPOSURE_MODE_AUTO);
        if (res == uvc::UVC_SUCCESS)
        {
            return;
        }

        if (res == uvc::UVC_ERROR_PIPE)
        {
            res = uvc::uvc_set_ae_mode(device.h_device, EXPOSURE_MODE_APERTURE);
            if (res != uvc::UVC_SUCCESS)
            {
                // meh
            }
        }
    }
}


/* device setup */

namespace camera_usb
{
    static bool read_device_config(DeviceUVC& device)
    {        
        using FF = uvc::frame_format;

        if (!device.h_device)
        {
            assert(false && "Device not open");
            return false;
        }

        auto& config = device.config;

        auto format = get_frame_format(device);

        if (!format.ok)
        {
            return false;
        }

        // this may break if camera does not support dimensions
        config.frame_width = format.width;
        config.frame_height = format.height;
        config.fps = 10'000'000 / format.interval;

        cvt::u32_to_fcc(format.four_cc_bytes, config.format_code);

        config.pixel_format = cvt::fcc_to_pf(config.format_code);        

        return load_device_config(device);
    }
    
    
    static bool read_device_properties(DeviceUVC& device)
    {
        uvc::device_descriptor* desc;

        auto res = uvc::uvc_get_device_descriptor(device.p_device, &desc);
        if (res != uvc::UVC_SUCCESS)
        {
            assert(false && "Could not find device");
            return false;
        }

        qsnprintf(device.product_id, 5, "%04x", desc->idProduct);
        qsnprintf(device.vendor_id, 5, "%04x", desc->idVendor);
        qsnprintf(device.serial_number, 32, "%s", desc->serialNumber);        
            
        uvc::uvc_free_device_descriptor(desc);

        qsnprintf(device.label, 32, "%c", 'A');

        return true;
    }
}


/* enumerate */

namespace camera_usb
{
    static bool enumerate_devices(DeviceListUVC& list)
    {
        auto res = uvc::uvc_init(&list.context, NULL);
        if (res != uvc::UVC_SUCCESS)
        {
            //print_uvc_error(res, "uvc_init");
            uvc::uvc_exit(list.context);
            return false;
        }

        res = uvc::uvc_get_device_list(list.context, &list.device_list);
        if (res != uvc::UVC_SUCCESS)
        {
            //print_uvc_error(res, "uvc_get_device_list");
            uvc::uvc_exit(list.context);
            return false;
        }

        if (!list.device_list[0])
        {
            uvc::uvc_exit(list.context);
            return false;
        }

        list.count = 0;
        for (int i = 0; list.device_list[i]; ++i) 
        {
            auto& device = list.devices[list.count];

            device.device_id = i;
            device.p_device = list.device_list[i];           

            read_device_properties(device);

            ++list.count;
        }

        if (list.count)
        {
            print_device_permissions_msg(list);
        }

        return list.count > 0;
    }


    static void close_devices(DeviceListUVC& list)
    {
        for (int i = 0; i < list.count; ++i)
        {
            auto& device = list.devices[i];
            close_stream(device);
            close_device(device);
            uvc::uvc_unref_device(device.p_device);
            device.p_device = nullptr;
        }

        if (list.device_list)
        {
            uvc::uvc_free_device_list(list.device_list, 0);
        }

        if (list.context)
        {
            uvc::uvc_exit(list.context);
        }
    }
}


/*  */

namespace camera_usb
{
    static bool open_device_stream(DeviceUVC& device)
    {
        if (!open_device(device))
        {
            assert(false && "Could not open device");
            return false;
        }

        if (!read_device_config(device))
        {
            assert(false && "Error getting device configuration");
            close_device(device);
            return false;
        }

        if (!open_stream(device))
        {
            assert(false && "Could not open stream");
            return false;
        }

        return true;
    }


    static bool start_device_stream(DeviceUVC& device)
    {
        if (!start_stream_single_frame(device))
        {
            assert(false && "Could not start stream");
            return false;
        }

        enable_exposure_mode(device);

        return true;
    }


    static void close_device_stream(DeviceUVC& device)
    {
        close_stream(device);
    }


    static bool grab_and_convert_frame_rgba(DeviceUVC& device)
    {
        uvc::frame* frame;

        auto res = uvc::uvc_stream_get_frame(device.h_stream, &frame);
        if (res != uvc::UVC_SUCCESS)
        {  
            return false;
        }

        auto span = span::make_view(frame->data, frame->data_bytes);

        auto format = device.config.pixel_format;
        auto w = device.config.frame_width;
        auto h = device.config.frame_height;

        cvt::to_yuv(span, w, h, device.yuv, format);
        cvt::yuv_to_rgba(device.yuv, device.rgba);
        
        return res == uvc::UVC_SUCCESS;
    }
}


/* static devices */

namespace camera_usb
{
    DeviceListUVC uvc_list;
}


/* api */

namespace camera_usb
{
    CameraList enumerate_cameras()
    {
        CameraList cameras;
        cameras.status = ConnectionStatus::Connecting;

        auto n_pixels = WIDTH_MAX * HEIGHT_MAX;
        uvc_list.data32 = img::create_buffer32(4 * n_pixels, "uvc data32");
        if (!uvc_list.data32.ok)
        {
            cameras.count = 0;
            cameras.status = ConnectionStatus::Disconnected;
            return cameras;
        }

        if (!enumerate_devices(uvc_list))
        {
            cameras.count = 0;
            cameras.status = ConnectionStatus::Disconnected;
            return cameras;
        }

        cameras.count = uvc_list.count;
        for (u32 i = 0; i < cameras.count; i++)
        {
            auto& camera = cameras.list[i];
            auto& device = uvc_list.devices[i];

            camera.id = device.device_id;
            camera.status = CameraStatus::Active;

            camera.vendor = span::to_string_view(device.vendor_id);
            camera.product = span::to_string_view(device.product_id);
            camera.serial_number = span::to_string_view(device.serial_number);
            camera.label = span::to_string_view(device.label);

            camera.format = span::to_string_view("XXXX");
        }

        cameras.status = ConnectionStatus::Connected;

        return cameras;
    }


    void close(CameraList& cameras)
    {
        mb::destroy_buffer(uvc_list.data32);
        close_devices(uvc_list);

        for (u32 i = 0; i < cameras.count; i++)
        {
            auto& camera = cameras.list[i];
            camera.id = -1;
            camera.status = CameraStatus::Inactive;
        }
    }
  
    
    bool open_camera(Camera& camera)
    {
        camera.busy = 1;

        auto& device = uvc_list.devices[camera.id];
        if (!open_device_stream(device))
        {        
            camera.busy = 0;    
            return false;
        }
        
        auto& config = device.config;
        camera.frame_width = config.frame_width;
        camera.frame_height = config.frame_height;
        camera.fps = config.fps;
        camera.format = span::to_string_view(config.format_code);

        if (!start_device_stream(device))
        {
            camera.busy = 0;
            return false;
        }

        // only one at a time
        auto& buffer = uvc_list.data32;
        auto w = camera.frame_width;
        auto h = camera.frame_height;
        mb::reset_buffer(buffer);
        device.rgba = img::make_view(w, h, buffer);
        device.yuv = convert::make_view_yuv(w, h, buffer);

        if (!buffer.ok)
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
        auto& device = uvc_list.devices[camera.id];
        
        device.grab_sw.start();

        if (grab_and_convert_frame_rgba(device))
        {
            img::copy(device.rgba, dst);
        }
        else
        {
            img::fill(dst, img::to_pixel(0, 0, 255));
        }

        device.grab_ms = device.grab_sw.get_time_milli();
        camera.fps = num::round_to_unsigned<u32>(1000.0 / device.grab_ms);

        camera.busy = 0;
    }


    void stream_camera(Camera& camera, grab_cb const& on_grab, bool_fn const& stream_condition)
    {
        auto& device = uvc_list.devices[camera.id];

        auto c_status = camera.status;

        camera.status = CameraStatus::Streaming;

        while (stream_condition())
        {
            device.grab_sw.start();
            if (grab_and_convert_frame_rgba(device))
            {
                on_grab(device.rgba);
            }
            device.grab_ms = device.grab_sw.get_time_milli();
            camera.fps = num::round_to_unsigned<u32>(1000.0 / device.grab_ms);
        }

        camera.status = c_status;
    }
}

#include "../image/convert.cpp"

#define LIBUVC_IMPLEMENTATION
#define LIBUVC_NUM_TRANSFER_BUFS 50
#define LIBUVC_TRACK_MEMORY
#include "libuvc3.hpp"