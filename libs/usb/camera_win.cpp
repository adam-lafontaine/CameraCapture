#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif


#include <Windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <Mfreadwrite.h>
#include <Shlwapi.h>


namespace w32
{
    constexpr u32 fcc_to_u32(cstr code)
    {
        u32 value = 0;
        
        for (u32 i = 0; i < 4; i++)
        {
            value |= (u8)code[i] << i * 8;
        }

        return value;
    }


    static void u32_to_fcc(u32 src, char* dst)
    {
        union
        {
            u32 value;
            char fcc[4];
        } sd;

        sd.value = src;

        for (u32 i = 0; i < 4; i++)
        {
            dst[i] = sd.fcc[i];
        }
    }


    enum class PixelFormat : u32
    {
        RGB32  = 22, // D3DFMT_X8R8G8B8
        ARGB32 = 21, // D3DFMT_A8R8G8B8
        RGB24  = 20, // D3DFMT_R8G8B8
        RGB555 = 25, // D3DFMT_X1R5G5B5
        RGB565 = 23, // D3DFMT_R5G6B5
        RGB8   = 41, // D3DFMT_P8
        L8     = 50, // D3DFMT_L8
        L16    = 81, // D3DFMT_L16
        D16    = 80, // D3DFMT_D16

        AI44 = fcc_to_u32("AI44"),
        AYUV = fcc_to_u32("AYUV"),
        YUY2 = fcc_to_u32("YUY2"),
        YVYU = fcc_to_u32("YVYU"),
        YVU9 = fcc_to_u32("YVU9"),
        UYVY = fcc_to_u32("UYVY"),
        NV11 = fcc_to_u32("NV11"),
        NV12 = fcc_to_u32("NV12"),
        YV12 = fcc_to_u32("YV12"),
        I420 = fcc_to_u32("I420"),
        IYUV = fcc_to_u32("IYUV"),
        Y210 = fcc_to_u32("Y210"),
        Y216 = fcc_to_u32("Y216"),
        Y410 = fcc_to_u32("Y410"),
        Y416 = fcc_to_u32("Y416"),
        Y41P = fcc_to_u32("Y41P"),
        Y41T = fcc_to_u32("Y41T"),
        Y42T = fcc_to_u32("Y42T"),
        P210 = fcc_to_u32("P210"),
        P216 = fcc_to_u32("P216"),
        P010 = fcc_to_u32("P010"),
        P016 = fcc_to_u32("P016"),
        v210 = fcc_to_u32("v210"),
        v216 = fcc_to_u32("v216"),
        v410 = fcc_to_u32("v410"),
        MP43 = fcc_to_u32("MP43"),
        MP4S = fcc_to_u32("MP4S"),
        M4S2 = fcc_to_u32("M4S2"),
        MP4V = fcc_to_u32("MP4V"),
        WMV1 = fcc_to_u32("WMV1"),
        WMV2 = fcc_to_u32("WMV2"),
        WMV3 = fcc_to_u32("WMV3"),
        WVC1 = fcc_to_u32("WVC1"),
        MSS1 = fcc_to_u32("MSS1"),
        MSS2 = fcc_to_u32("MSS2"),
        MPG1 = fcc_to_u32("MPG1"),
        DVSL = fcc_to_u32("dvsl"),
        DVSD = fcc_to_u32("dvsd"),
        DVHD = fcc_to_u32("dvhd"),
        DV25 = fcc_to_u32("dv25"),
        DV50 = fcc_to_u32("dv50"),
        DVH1 = fcc_to_u32("dvh1"),
        DVC  = fcc_to_u32("dvc "),
        H264 = fcc_to_u32("H264"),
        H265 = fcc_to_u32("H265"),
        MJPG = fcc_to_u32("MJPG"),
        _420O = fcc_to_u32("420O"),
        HEVC = fcc_to_u32("HEVC"),
        HEVC_ES = fcc_to_u32("HEVS"),
        VP80 = fcc_to_u32("VP80"),
        VP90 = fcc_to_u32("VP90"),
        ORAW = fcc_to_u32("ORAW"),
        H263 = fcc_to_u32("H263"),

        A2R10G10B10   = 31,  // D3DFMT_A2B10G10R10
        A16B16G16R16F = 113, // D3DFMT_A16B16G16R16F

        VP10 = fcc_to_u32("VP10"),
        AV1 = fcc_to_u32("AV01"),

        Unknown = fcc_to_u32("XXXX")
    };


    static void pf_to_fcc(PixelFormat pf, char* dst)
    {
        u32_to_fcc((u32)pf, dst);
    }



}


/* types */

namespace w32
{
    using Device_p = IMFActivate*;
    using MediaSource_p = IMFMediaSource*;
    using SourceReader_p = IMFSourceReader*;
    using Buffer_p = IMFMediaBuffer*;
    using Sample_p = IMFSample*;


    class Frame
    {
    public:
        DWORD stream_index = 0;
        DWORD flags = 0;
        LONGLONG timestamp = 0;

        Buffer_p buffer = nullptr;

        BYTE* data = nullptr;
        DWORD size_bytes = 0;

        bool is_locked = false;
    };


    class FrameFormat
    {
    public:
        UINT32 width = 0;
        UINT32 height = 0;
        UINT32 stride = 0;
        UINT32 fps = 0;
        //UINT32 pixel_size = 0;

        PixelFormat pixel_format = PixelFormat::Unknown;
        char format_code[5] = { 0 };
    };


    template <typename T>
    class DataResult
    {
    public:
        T data;
        bool success = false;
    };


    union Bytes8
    {
        UINT64 val64 = 0;

        struct
        {
            UINT32 lo32;
            UINT32 hi32;
        };
    };


    template <class T>
    static void release(T*& ptr)
    {
        if (ptr)
        {
            ptr->Release();
            ptr = nullptr;
        }
    }
}


namespace w32
{
    static bool init()
    {
        HRESULT hr = MFStartup(MF_VERSION);

        return hr == S_OK;
    }


    static void shutdown()
    {
        MFShutdown();
    }
}


/* read_frame */

namespace w32
{
    static DataResult<Frame> read_frame(SourceReader_p reader, Sample_p& sample)
    {
        DataResult<Frame> result{};
        auto& frame = result.data;

        HRESULT hr = reader->ReadSample(
            (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, 
            0, 
            &frame.stream_index, 
            &frame.flags, 
            &frame.timestamp, 
            &sample);

        if (FAILED(hr))
        {
            return result;
        }

        hr = sample->ConvertToContiguousBuffer(&frame.buffer);
        if (FAILED(hr))
        {
            return result;
        }

        hr = frame.buffer->Lock(&frame.data, NULL, &frame.size_bytes);
        if (FAILED(hr))
        {
            release(frame.buffer);
            return result;
        }

        frame.is_locked = true;

        result.success = true;
        return result;
    }


    static void release(Frame& frame)
    {
        if (frame.is_locked)
        {
            frame.buffer->Unlock();
            frame.is_locked = false;
        }

        release(frame.buffer);
    }
}


/* ? */

namespace w32
{
    static bool activate(Device_p device, MediaSource_p& source, SourceReader_p& reader)
    {
        HRESULT hr = device->ActivateObject(__uuidof(IMFMediaSource), (void**)&source);
        if (FAILED(hr))
        {
            return false;
        }

        hr = MFCreateSourceReaderFromMediaSource(source, NULL, &reader);
        if (FAILED(hr))
        {
            release(source);
            return false;
        }

        hr = reader->SetStreamSelection(MF_SOURCE_READER_FIRST_VIDEO_STREAM, TRUE);
        if (FAILED(hr))
        {
            release(source);
            release(reader);
            return false;
        }

        Frame frame{};
        Sample_p sample = nullptr;

        hr = reader->ReadSample(
            (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, 
            0, 
            &frame.stream_index, 
            &frame.flags, 
            &frame.timestamp, 
            &sample);

        if (FAILED(hr))
        {
            release(source);
            release(reader);
            return false;
        }

        return true;
    }


    

    static DataResult<FrameFormat> get_frame_format(SourceReader_p reader)
    {
        DataResult<FrameFormat> result{};
        auto& format = result.data;

        IMFMediaType* media_type = nullptr;

        HRESULT hr = reader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, &media_type);
        if (FAILED(hr))
        {
            return result;
        }

        GUID major_type;
        hr = media_type->GetGUID(MF_MT_MAJOR_TYPE, &major_type);
        if (FAILED(hr) || major_type != MFMediaType_Video)
        {
            release(media_type);
            return result;
        }

        hr = MFGetAttributeSize(media_type, MF_MT_FRAME_SIZE, &format.width, &format.height);
        if (FAILED(hr) || !format.width || ! format.height)
        {
            release(media_type);
            return result;
        }

        hr = media_type->GetUINT32(MF_MT_DEFAULT_STRIDE, &format.stride);
        if (FAILED(hr) || !format.stride)
        {
            //release(media_type);
            //return result;
        }

        Bytes8 fps;

        hr = media_type->GetUINT64(MF_MT_FRAME_RATE, &fps.val64);

        if (FAILED(hr) || !fps.hi32 || !fps.lo32)
        {
            release(media_type);
            return result;
        }

        format.fps = fps.hi32 / fps.lo32;
        //format.pixel_size = format.stride / format.width;

        GUID sub_type;
        hr = media_type->GetGUID(MF_MT_SUBTYPE, &sub_type);
        if (FAILED(hr))
        {
            release(media_type);
            return result;
        }

        format.pixel_format = (PixelFormat)sub_type.Data1;
        pf_to_fcc(format.pixel_format, format.format_code);

        release(media_type);
        result.success = true;
        return result;
    } 
}


#include "camera_usb.hpp"
#include "mjpeg_convert.hpp"
#include "../qsprintf/qsprintf.hpp"
#include "../util/numeric.hpp"
#include "../util/stopwatch.hpp"

namespace convert
{
    namespace img = image;


    static void mjpeg_to_rgba(w32::Frame& frame, img::ImageView const& dst)
    {
        auto format = mjpeg::image_format::RGBA8;

        mjpeg::convert((u8*)frame.data, dst.width, (u32)frame.size_bytes, (u8*)dst.matrix_data_, format);
    }


    static void yuv_to_rgba(u8 y, u8 u, u8 v, img::Pixel* dst)
    {
        constexpr f32 yr = 1.0f;
        constexpr f32 ur = 0.0f;
        constexpr f32 vr = 1.13983f;

        constexpr f32 yg = 1.0f;
        constexpr f32 ug = -0.39465f;
        constexpr f32 vg = -0.5806f;

        constexpr f32 yb = 1.0f;
        constexpr f32 ub = 2.03211f;
        constexpr f32 vb = 0.0f;

        constexpr f32 c = 1.0f / 255.0f;

        auto yc = y * c;
        auto uc = u * c - 0.5f;
        auto vc = v * c - 0.5f;

        auto r = num::clamp((yr * yc) + (ur * uc) + (vr * vc), 0.0f, 1.0f) * 255;
        auto g = num::clamp((yg * yc) + (ug * uc) + (vg * vc), 0.0f, 1.0f) * 255;
        auto b = num::clamp((yb * yc) + (ub * uc) + (vb * vc), 0.0f, 1.0f) * 255;

        dst->red = num::round_to_unsigned<u8>(r);
        dst->green = num::round_to_unsigned<u8>(g);
        dst->blue = num::round_to_unsigned<u8>(b);
        dst->alpha = 255;
    }


    static void yuyv_to_rgba(w32::Frame& frame, img::ImageView const& dst)
    {
        auto const len  = dst.width * dst.height;

        auto yuyv = (u8*)frame.data;
        auto d = dst.matrix_data_;

        auto y1 = yuyv;
        auto u = y1 + 1;
        auto y2 = u + 1;
        auto v = y2 + 1;

        auto d1 = d;
        auto d2 = d1 + 1;

        for (u32 i = 0; i < len; i += 4)
        {
            yuv_to_rgba(*y1, *u, *v, d1);
            yuv_to_rgba(*y2, *u, *v, d2);

            y1 += 4;
            u += 4;
            y2 += 4;
            v += 4;

            d1 += 1;
            d2 += 1;
        }
    }


    static void yvyu_to_rgba(w32::Frame& frame, img::ImageView const& dst)
    {
        auto const len  = dst.width * dst.height;

        auto yuyv = (u8*)frame.data;
        auto d = dst.matrix_data_;

        auto y1 = yuyv;
        auto v = y1 + 1;
        auto y2 = v + 1;
        auto u = y2 + 1;

        auto d1 = d;
        auto d2 = d1 + 1;

        for (u32 i = 0; i < len; i += 4)
        {
            yuv_to_rgba(*y1, *u, *v, d1);
            yuv_to_rgba(*y2, *u, *v, d2);

            y1 += 4;
            v += 4;
            y2 += 4;
            u += 4;

            d1 += 1;
            d2 += 1;
        }
    }


    static void uyvy_to_rgba(w32::Frame& frame, img::ImageView const& dst)
    {
        auto const len  = dst.width * dst.height;

        auto uyvy = (u8*)frame.data;
        auto d = dst.matrix_data_;

        auto u = uyvy;
        auto y1 = u + 1;
        auto v = y1 + 1;
        auto y2 = v + 1;

        auto d1 = d;
        auto d2 = d1 + 1;

        for (u32 i = 0; i < len; i += 4)
        {
            yuv_to_rgba(*y1, *u, *v, d1);
            yuv_to_rgba(*y2, *u, *v, d2);

            y1 += 4;
            v += 4;
            y2 += 4;
            u += 4;

            d1 += 1;
            d2 += 1;
        }
    }
    
    
    static void nv12_to_rgba(w32::Frame& frame, img::ImageView const& dst)
    {
        auto const width = dst.width;
        auto const height = dst.height;

        assert(frame.size_bytes == width * height + width * height / 2);

        auto sy = (u8*)frame.data;
        auto suv = sy + width * height;
        auto sd = dst.matrix_data_;

        auto u = suv;
        auto v = u + 1;        

        for (u32 h = 0; h < height; h += 2)
        {
            auto y1 = sy + h * width;
            auto y2 = y1 + 1;
            auto y3 = y1 + width;
            auto y4 = y3 + 1;

            auto d1 = sd + h * width;
            auto d2 = d1 + 1;
            auto d3 = d1 + width;
            auto d4 = d3 + 1;

            for (u32 w = 0; w < width; w += 2)
            {  
                yuv_to_rgba(*y1, *u, *v, d1);
                yuv_to_rgba(*y2, *u, *v, d2);
                yuv_to_rgba(*y3, *u, *v, d3);
                yuv_to_rgba(*y4, *u, *v, d4);

                y1 += 2;
                y2 += 2;
                y3 += 2;
                y4 += 2;

                d1 += 2;
                d2 += 2;
                d3 += 2;
                d4 += 2;

                u += 2;
                v += 2;
            }
        }
    }

}


namespace convert
{
    using PF = w32::PixelFormat;


    static void convert(w32::Frame& frame, img::ImageView const& dst, PF format)
    {
        switch (format)
        {
        case PF::YUY2:
            yuyv_to_rgba(frame, dst);
            break;
        
        case PF::YVYU:
            yvyu_to_rgba(frame, dst);
            break;
        
        case PF::UYVY:
            uyvy_to_rgba(frame, dst);
            break;

        case PF::NV12:
            nv12_to_rgba(frame, dst);
            break;
        
        case PF::MJPG:
            mjpeg_to_rgba(frame, dst);
            break;

        default:
            img::fill(dst, img::to_pixel(100));
        }
    }
}


namespace camera_usb
{
    namespace img = image;

    constexpr u8 DEVICE_COUNT_MAX = 16;


    class DeviceW32
    {
    public:
        int device_id = -1;

        w32::Device_p p_device = nullptr;
        w32::MediaSource_p p_source = nullptr;
        w32::SourceReader_p p_reader = nullptr;

        w32::Sample_p p_sample = nullptr;

        w32::FrameFormat format;

        Stopwatch grab_sw;
        f32 grab_ms;

        img::ImageView rgba;
    };


    class DeviceListW32
    {
    public:
        w32::Device_p* device_list = nullptr;

        DeviceW32 devices[DEVICE_COUNT_MAX] = { 0 };

        u32 count = 0;

        img::Buffer32 rgba_data;
    };
}


/* wrappers */

namespace camera_usb
{
    static void close_device(DeviceW32& device)
    {
        w32::release(device.p_source);
        w32::release(device.p_reader);
        w32::release(device.p_device);
    }


    static bool open_device(DeviceW32& device)
    {
        return w32::activate(device.p_device, device.p_source, device.p_reader);
    }


    static bool read_device_format(DeviceW32& device)
    {
        auto result = w32::get_frame_format(device.p_reader);
        if (!result.success)
        {
            return false;
        }

        device.format = result.data;
        return true;
    }


    static bool grab_and_convert_frame_rgba(DeviceW32& device)
    {
        auto result = w32::read_frame(device.p_reader, device.p_sample);
        if (!result.success)
        {
            return false;
        }

        auto& frame = result.data;

        convert::convert(frame, device.rgba, device.format.pixel_format);

        w32::release(frame);
        w32::release(device.p_sample);

        return true;
    }
}


/* device setup */

namespace camera_usb
{
    static bool read_device_properties(DeviceW32& device)
    {
        // TODO
        return true;
    }


    static bool connect_device(DeviceW32& device)
    {
        if (!open_device(device))
        {
            assert(false && "Could not open device");
            return false;
        }

        if (!read_device_format(device))
        {
            assert(false && "Error getting device configuration");
            close_device(device);
            return false;
        }

        return true;
    }
}


/* enumerate */

namespace camera_usb
{
    static bool enumerate_devices(DeviceListW32& list)
    {
        if (!w32::init())
        {
            return false;
        }

        HRESULT hr = S_OK;

        IMFAttributes* p_attr = nullptr;

        hr = MFCreateAttributes(&p_attr, 1);
        if (FAILED(hr))
        {
            return false;
        }

        hr = p_attr->SetGUID(
            MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
            MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID
        );

        if (FAILED(hr))
        {
            w32::release(p_attr);
            return false;
        }

        u32 n_devices = 0;

        hr = MFEnumDeviceSources(p_attr, &list.device_list, &n_devices);
        w32::release(p_attr);

        if (FAILED(hr) || !n_devices)
        {      
            return false;
        }

        list.count = n_devices;
        assert(n_devices <= DEVICE_COUNT_MAX);

        for (u32 i = 0; i < n_devices; ++i)
        {
            auto& device = list.devices[i];

            device.p_device = list.device_list[i];
            device.device_id = i;
        }

        return list.count > 0;
    }


    static void close_devices(DeviceListW32& list)
    {
        for (int i = 0; i < list.count; ++i)
        {
            auto& device = list.devices[i];
            close_device(device);            
        }

        CoTaskMemFree(list.device_list);
        w32::shutdown();
    }
}


namespace camera_usb
{
    static DeviceListW32 w32_list;
}


/* api */

namespace camera_usb
{
    CameraList enumerate_cameras()
    {
        CameraList cameras{};
        cameras.status = ConnectionStatus::Connecting;

        auto n_pixels = WIDTH_MAX * HEIGHT_MAX;
        w32_list.rgba_data = img::create_buffer32(n_pixels, "w32 rgba");
        if (!w32_list.rgba_data.ok)
        {
            cameras.count = 0;
            cameras.status = ConnectionStatus::Disconnected;
            return cameras;
        }

        if (!enumerate_devices(w32_list))
        {
            cameras.count = 0;
            cameras.status = ConnectionStatus::Disconnected;
            return cameras;
        }

        cameras.count = w32_list.count;
        for (u32 i = 0; i < cameras.count; i++)
        {
            auto& camera = cameras.list[i];
            auto& device = w32_list.devices[i];

            camera.id = device.device_id;
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
        close_devices(w32_list);

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

        auto& device = w32_list.devices[camera.id];
        if (!connect_device(device))
        {        
            camera.busy = 0;    
            return false;
        }

        auto& format = device.format;
        camera.frame_width = format.width;
        camera.frame_height = format.height;
        camera.fps = format.fps;
        
        camera.format = span::to_string_view(format.format_code);

        // only one at a time
        mb::reset_buffer(w32_list.rgba_data);

        device.rgba = img::make_view(camera.frame_width, camera.frame_height, w32_list.rgba_data);

        camera.status = CameraStatus::Open;
        camera.busy = 0;

        return true;
    }


    void grab_image(Camera& camera, img::ImageView const& dst)
    {
        camera.busy = 1;
        auto& device = w32_list.devices[camera.id];
        
        device.grab_sw.start();

        if (grab_and_convert_frame_rgba(device))
        {
            img::copy(device.rgba, dst);
        }

        device.grab_ms = device.grab_sw.get_time_milli();
        camera.fps = num::round_to_unsigned<u32>(1000.0 / device.grab_ms);

        camera.busy = 0;
    }


    void stream_camera(Camera& camera, grab_cb const& on_grab, bool_fn const& stream_condition)
    {
        auto& device = w32_list.devices[camera.id];

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


#define MJPEG_CONVERT_IMPLEMENTATION
#include "mjpeg_convert.hpp"