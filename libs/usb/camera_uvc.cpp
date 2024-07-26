#pragma once

#include "camera_usb.hpp"
#include "libuvc2.hpp"
#include "../qsprintf/qsprintf.hpp"

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

        bool is_connected = false;
        bool is_streaming = false;
    };


    class DeviceListUVC
    {
    public:
        uvc::context* context = nullptr;
        uvc::device** device_list = nullptr;

        DeviceUVC devices[DEVICE_COUNT_MAX] = { 0 };

        u32 count = 0;

    };


    class CameraUVC
    {
    public:
        
    };
}


/*  */

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


    static bool read_device_config(DeviceUVC& device)
    {
        auto& config = device.config;

        const uvc::format_desc* format_desc = uvc::uvc_get_format_descs(device.h_device);
        const uvc::frame_desc* frame_desc = format_desc->frame_descs;

        if (!frame_desc) 
        { 
            return false;
        }

        //format_desc->bBitsPerPixel;

        config.frame_width = frame_desc->wWidth;
        config.frame_height = frame_desc->wHeight;
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

        auto res = uvc::uvc_get_stream_ctrl_format_size(
            device.h_device, &device.ctrl, // result stored in ctrl
            config.format,
            config.frame_width, 
            config.frame_height, 
            config.fps);

        return res >= 0;
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
        
        if (!open_device(device))
        {
            assert(false && "Could not open device");
            return false;
        }

        if (!read_device_config(device))
        {
            assert(false && "Error getting device configuration");
            return false;
        }

        close_device(device);

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
            auto p_device = list.device_list[i]; 
            auto& device = list.devices[list.count];

            device.device_id = i;
            device.p_device = p_device;           

            read_device_properties(device);

            ++list.count;
        }

        return list.count > 0;
    }


    static void close_devices(DeviceListUVC& list)
    {
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

        if (!enumerate_devices(uvc_list))
        {
            cameras.count = 0;
            return cameras;
        }



        cameras.count = uvc_list.count;
        for (u32 i = 0; i < cameras.count; i++)
        {
            auto& camera = cameras.list[i];
            auto& device = uvc_list.devices[i];
            auto& config = device.config;

            camera.id = device.device_id;

            camera.frame_width = config.frame_width;
            camera.frame_height = config.frame_height;
            camera.fps = config.fps;
            camera.format = span::to_string_view(config.format_code);

            camera.vendor = span::to_string_view(device.vendor_id);
            camera.product = span::to_string_view(device.product_id);
            camera.serial_number = span::to_string_view(device.serial_number);
            camera.label = span::to_string_view(device.label);
        }

        return cameras;
    }


    void close()
    {
        close_devices(uvc_list);
    }
}


#define LIBUVC_IMPLEMENTATION
#include "libuvc2.hpp"