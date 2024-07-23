#include "camera_usb.hpp"
#include "libuvc2.hpp"


namespace camera_usb
{
    namespace img = image;


    constexpr u8 DEVICE_COUNT_MAX = 16;


    class DeviceUVC
    {
    public:
        uvc::device* p_device = nullptr;
        uvc::device_handle* h_device = nullptr;
        uvc::stream_ctrl ctrl;
        uvc::stream_handle* h_stream = nullptr;

        int device_id = -1;

        int product_id = -1;
        int vendor_id = -1;
        
        int frame_width = -1;
        int frame_height = -1;
        int fps = -1;

        uvc::uvc_frame_format format;
        u32 format_code;

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
        
        return res != uvc::UVC_SUCCESS;
    }
    
    
    static void close_device(DeviceUVC& device)
    {
        if (device.h_device)
        {
            uvc::uvc_close(device.h_device);
            device.h_device = nullptr;
        } 
    }


    static bool read_device_properties(DeviceUVC& device)
    {
        if (!open_device(device))
        {
            return false;
        }

        int width = 640;
        int height = 480;
        int fps = 30;

        const uvc::format_desc* format_desc = uvc::uvc_get_format_descs(device.h_device);
        const uvc::frame_desc* frame_desc = format_desc->frame_descs;

        if (frame_desc) 
        {
            width = frame_desc->wWidth;
            height = frame_desc->wHeight;
            fps = 10'000'000 / frame_desc->dwDefaultFrameInterval;
        }

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

        device.format = frame_format;

        device.format_code = *(u32*)format_desc->fourccFormat;

        device.frame_width = width;
        device.frame_height = height;
        device.fps = fps;

        /*auto res = uvc::uvc_get_stream_ctrl_format_size(
            device.h_device, &device.ctrl, // result stored in ctrl
            frame_format,
            width, height, fps // width, height, fps
        );*/

        close_device(device);

        return true;
    }
}


/* enumerate */

namespace camera_usb
{
    static bool enumerate_devices(DeviceListUVC& list)
    {
        uvc::device_descriptor* desc;

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

            res = uvc::uvc_get_device_descriptor(p_device, &desc);
            if (res != uvc::UVC_SUCCESS)
            {
                //print_uvc_error(res, "uvc_get_device_descriptor");
                continue;
            }

            auto& device = list.devices[list.count];

            device.device_id = i;
            device.p_device = p_device;
            device.product_id = desc->idProduct;
            device.vendor_id = desc->idVendor;
            
            uvc::uvc_free_device_descriptor(desc);

            read_device_properties(device);

            ++list.count;
        }

        return list.count > 0;
    }
}


/* connect disconnect */

namespace camera_usb
{
    
}