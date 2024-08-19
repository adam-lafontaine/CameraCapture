#pragma once

#include "camera_usb.hpp"
#include "../image/convert.hpp"
#include "../qsprintf/qsprintf.hpp"
#include "../util/numeric.hpp"
#include "../util/stopwatch.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <Mfreadwrite.h>
#include <Shlwapi.h>


/* types */

namespace w32
{
    namespace cvt = convert;

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

        cvt::PixelFormat pixel_format = cvt::PixelFormat::Unknown;
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
            
        release(sample);

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
        if (FAILED(hr) || !format.width || !format.height)
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

        release(media_type);

        auto pf = (cvt::PixelFormat)sub_type.Data1;
        pf_to_fcc(pf, format.format_code);
        
        Sample_p sample = nullptr;

        auto read_result = read_frame(reader, sample);
        if (!read_result.success)
        {
            return result;
        }

        auto& frame = read_result.data;

        format.pixel_format = cvt::validate_format(frame.size_bytes, format.width, format.height, pf);

        pf_to_fcc(format.pixel_format, format.format_code);

        release(frame);
        release(sample);
        
        result.success = true;
        return result;
    }
}


namespace camera_usb
{
    namespace img = image;
    namespace cvt = convert;

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
        convert::ViewYUV yuv;
    };


    class DeviceListW32
    {
    public:
        w32::Device_p* device_list = nullptr;

        DeviceW32 devices[DEVICE_COUNT_MAX] = { 0 };

        u32 count = 0;

        img::Buffer32 data32;
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
        auto span = span::make_view((u8*)frame.data, frame.size_bytes);

        cvt::to_yuv(span, device.yuv, device.format.pixel_format);
        cvt::yuv_to_rgba(device.yuv, device.rgba);

        cvt::convert_view(span, device.rgba, device.format.pixel_format);

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
        w32_list.data32 = img::create_buffer32(4 * n_pixels, "w32 data32");
        if (!w32_list.data32.ok)
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
        auto& buffer = w32_list.data32;
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


#include "../image/convert.cpp"