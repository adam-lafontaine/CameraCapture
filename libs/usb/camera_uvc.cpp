#pragma once

#include "camera_usb.hpp"
#include "libuvc3.hpp"
#include "../qsprintf/qsprintf.hpp"
#include "../util/numeric.hpp"

#include <cassert>

namespace num = numeric;

/*

libuvc requires RW permissions for opening capturing devices, so you must
create the following .rules file:

/etc/udev/rules.d/99-uvc.rules

Then, for each webcam add the following line:

SUBSYSTEMS=="usb", ENV{DEVTYPE}=="usb_device", ATTRS{idVendor}=="XXXX", ATTRS{idProduct}=="YYYY", MODE="0666"

Replace XXXX and YYYY for the 4 hexadecimal characters corresponding to the
vendor and product ID of your webcams.

*/


namespace camera_usb
{
    namespace img = image;


    constexpr u8 DEVICE_COUNT_MAX = 16;


    enum class DeviceStatus : u8
    {
        Inactive = 0,
        Active,
        DeviceOpen,
        StreamOpen,
        StreamReady,
        StreamRunning
    };


    class DeviceConfigUVC
    {
    public:
        u32 frame_width = 0;
        u32 frame_height = 0;
        u32 fps = 0;

        uvc::uvc_frame_format format;
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

        char product_id[5] = { 0 };
        char vendor_id[5] = { 0 };
        char serial_number[32] = { 0 };

        char label[32] = { 0 };
        
        DeviceConfigUVC config;

        DeviceStatus status = DeviceStatus::Inactive;

        img::ImageView rgba;
    };


    class DeviceListUVC
    {
    public:
        uvc::context* context = nullptr;
        uvc::device** device_list = nullptr;

        DeviceUVC devices[DEVICE_COUNT_MAX] = { 0 };

        u32 count = 0;

        img::Buffer32 rgba_data;

    };


    class CameraUVC
    {
    public:
        
    };
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


    static bool load_device_config(DeviceUVC& device)
    {
        if (!device.h_device)
        {
            assert(false && "Device not open");
            return false;
        }

        // test to see if config exists
        auto res = uvc::uvc_get_stream_ctrl_format_size(
            device.h_device, 
            &device.ctrl, // result stored in ctrl
            device.config.format,
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
        constexpr u8 EXPOSURE_MODE_AUTO = 2;
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
        if (!device.h_device)
        {
            assert(false && "Device not open");
            return false;
        }

        auto& config = device.config;

        const uvc::format_desc* format_desc = uvc::uvc_get_format_descs(device.h_device);
        const uvc::frame_desc* frame_desc = format_desc->frame_descs;

        if (!frame_desc) 
        { 
            return false;
        }

        //format_desc->bBitsPerPixel;

        // this may break if camera does not support dimensions
        config.frame_width = num::min(WIDTH_MAX, (u32)frame_desc->wWidth);
        config.frame_height = num::min(HEIGHT_MAX, (u32)frame_desc->wHeight);
        config.fps = 10'000'000 / frame_desc->dwDefaultFrameInterval;

        qsnprintf(config.format_code, 5, "%s", (char*)format_desc->fourccFormat);

        uvc::frame_format frame_format;        

        switch (format_desc->bDescriptorSubtype) 
        {
        case uvc::UVC_VS_FORMAT_MJPEG:
            frame_format = uvc::UVC_FRAME_FORMAT_MJPEG;
            break;
        
        case uvc::UVC_VS_FORMAT_FRAME_BASED:
            frame_format = uvc::UVC_FRAME_FORMAT_H264;
            break;

        default:
            frame_format = uvc::UVC_FRAME_FORMAT_YUYV;
            break;
        }

        config.format = frame_format;

        return load_device_config(device);
    }
    
    
    static bool read_device_properties(DeviceUVC& device)
    {
        static char label_ch = 'A';

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

        qsnprintf(device.label, 32, "%c", label_ch);
        ++label_ch;

        return true;
    }
}


/* enumerate */

namespace camera_usb
{
    static bool enumerate_activate_devices(DeviceListUVC& list)
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

        u32 max_pixels = 0;

        list.count = 0;
        for (int i = 0; list.device_list[i]; ++i) 
        {
            auto p_device = list.device_list[i]; 
            auto& device = list.devices[list.count];

            device.device_id = i;
            device.p_device = p_device;           

            read_device_properties(device);

            device.status = DeviceStatus::Active;

            ++list.count;

            max_pixels = num::max(max_pixels, device.config.frame_width * device.config.frame_height);
        }

        if (max_pixels)
        {
            list.rgba_data = img::create_buffer32(max_pixels, "uvc rbga_data");
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

            device.status = DeviceStatus::Inactive;
        }

        if (list.device_list)
        {
            uvc::uvc_free_device_list(list.device_list, 0);
        }

        if (list.context)
        {
            uvc::uvc_exit(list.context);
        }

        mb::destroy_buffer(list.rgba_data);
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

        device.status = DeviceStatus::StreamOpen;

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

        device.status = DeviceStatus::StreamReady;

        return true;
    }


    static void close_device_stream(DeviceUVC& device)
    {
        close_stream(device);

        if (device.status > DeviceStatus::DeviceOpen)
        {
            device.status = DeviceStatus::DeviceOpen;
        }
    }


    static bool grab_and_convert_frame_rgba(DeviceUVC& device)
    {
        if (device.status < DeviceStatus::StreamReady)
        {
            return false;
        }

        uvc::frame* in_frame;

        auto res = uvc::uvc_stream_get_frame(device.h_stream, &in_frame);
        if (res != uvc::UVC_SUCCESS)
        {  
            return false;
        }        

        auto dst = (u8*)device.rgba.matrix_data_;

        // todo
        res = uvc::opt::mjpeg2rgba(in_frame, dst);
        
        return res == uvc::UVC_SUCCESS;
    }
}


/* static devices */

namespace camera_usb
{
    DeviceListUVC uvc_list;


    static DeviceUVC find_device(Camera const& camera)
    {
        for (u32 i = 0; i < uvc_list.count; i++)
        {
            auto& device = uvc_list.devices[i];
            if (device.device_id == camera.id)
            {
                return device;
            }
        }

        DeviceUVC empty{};
        empty.device_id = -1;

        return empty;
    }
}


/* api */

namespace camera_usb
{
    CameraList enumerate_cameras()
    {
        CameraList cameras;
        cameras.status = ConnectionStatus::Connecting;

        auto n_pixels = WIDTH_MAX * HEIGHT_MAX;
        uvc_list.rgba_data = img::create_buffer32(n_pixels, "uvc rgba");
        if (!uvc_list.rgba_data.ok)
        {
            cameras.count = 0;
            cameras.status = ConnectionStatus::Disconnected;
            return cameras;
        }

        if (!enumerate_activate_devices(uvc_list))
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
        mb::reset_buffer(uvc_list.rgba_data);

        device.rgba = img::make_view(camera.frame_width, camera.frame_height, uvc_list.rgba_data);

        camera.status = CameraStatus::Open;
        camera.busy = 0;

        return true;
    }


    void grab_image(Camera& camera, img::ImageView const& dst)
    {
        camera.busy = 1;
        auto& device = uvc_list.devices[camera.id];

        if (grab_and_convert_frame_rgba(device))
        {
            img::copy(device.rgba, dst);
        }
        else
        {
            img::fill(dst, img::to_pixel(0, 0, 255));
        }

        camera.busy = 0;
    }


    void stream_camera(Camera& camera, grab_cb const& on_grab, bool_fn const& stream_condition)
    {
        auto& device = uvc_list.devices[camera.id];

        auto c_status = camera.status;
        auto d_status = device.status;

        camera.status = CameraStatus::Streaming;
        device.status = DeviceStatus::StreamRunning;

        while (stream_condition() && device.status == DeviceStatus::StreamRunning)
        {
            if (grab_and_convert_frame_rgba(device))
            {
                on_grab(device.rgba);
            }
        }

        if (device.status != DeviceStatus::Inactive)
        {
            camera.status = c_status;
            device.status = d_status;
        }
    }
}


#define LIBUVC_IMPLEMENTATION
#include "libuvc3.hpp"