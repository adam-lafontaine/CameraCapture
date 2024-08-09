#define NOMINMAX
#include <Windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <Mfreadwrite.h>
#include <Shlwapi.h>
#include <vector>

#ifdef MSVC

// TODO Makefile flags
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "Shlwapi.lib")

#endif


namespace w32
{
    enum class PixelFormat :  int
    {
        Unknown = -1,

        RGB32 = 1,
        ARGB32 = 2,
        RGB24 = 3,
        RGB555 = 4,
        RGB565 = 5,
        RGB8 = 6,
        L8 = 7,
        L16 = 8,
        D16 = 9,
        AI44 = 10,
        AYUV = 11,
        YUYV = 12,
        YVYU = 13,
        YVU9 = 14,
        UYVY = 15,
        NV11 = 16,
        NV12 = 17,
        YV12 = 18,
        I420 = 19,
        IYUV = 20,
        Y210 = 21,
        Y216 = 22,
        Y410 = 23,
        Y416 = 24,
        Y41P = 25,
        Y41T = 26,
        Y42T = 27,
        P210 = 28,
        P216 = 29,
        P010 = 30,
        P016 = 31,
        v210 = 32,
        v216 = 33,
        v410 = 34,
        MP43 = 35,
        MP4S = 36,
        M4S2 = 37,
        MP4V = 38,
        WMV1 = 39,
        WMV2 = 40,
        WMV3 = 41,
        WVC1 = 42,
        MSS1 = 43,
        MSS2 = 44,
        MPG1 = 45,
        DVSL = 46,
        DVSD = 47,
        DVHD = 48,
        DV25 = 49,
        DV50 = 50,
        DVH1 = 51,
        DVC = 52,
        H264 = 53,
        H265 = 54,
        MJPG = 55,
        _420O = 56,
        HEVC = 57,
        HEVC_ES = 58,
        VP80 = 59,
        VP90 = 60,
        ORAW = 61,
        H263 = 62,
        A2R10G10B10 = 63,
        A16B16G16R16F = 64,
        VP10 = 65,
        AV1 = 66,
    };
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

        if (sub_type == MFVideoFormat_RGB32) { format.pixel_format = PixelFormat::RGB32; }
        else if(sub_type == MFVideoFormat_ARGB32) { format.pixel_format = PixelFormat::ARGB32; }
        else if(sub_type == MFVideoFormat_RGB24)  { format.pixel_format = PixelFormat::RGB24; }
        else if(sub_type == MFVideoFormat_RGB555) { format.pixel_format = PixelFormat::RGB555; }
        else if(sub_type == MFVideoFormat_RGB565) { format.pixel_format = PixelFormat::RGB565; }
        else if(sub_type == MFVideoFormat_RGB8)   { format.pixel_format = PixelFormat::RGB8; }
        else if(sub_type == MFVideoFormat_L8)     { format.pixel_format = PixelFormat::L8; }
        else if(sub_type == MFVideoFormat_L16)  { format.pixel_format = PixelFormat::L16; }
        else if(sub_type == MFVideoFormat_D16)  { format.pixel_format = PixelFormat::D16; }
        else if(sub_type == MFVideoFormat_AI44) { format.pixel_format = PixelFormat::AI44; }
        else if(sub_type == MFVideoFormat_AYUV) { format.pixel_format = PixelFormat::AYUV; }
        else if(sub_type == MFVideoFormat_YUY2) { format.pixel_format = PixelFormat::YUYV; }
        else if(sub_type == MFVideoFormat_YVYU) { format.pixel_format = PixelFormat::YVYU; }
        else if(sub_type == MFVideoFormat_YVU9) { format.pixel_format = PixelFormat::YVU9; }
        else if(sub_type == MFVideoFormat_UYVY) { format.pixel_format = PixelFormat::UYVY; }
        else if(sub_type == MFVideoFormat_NV11) { format.pixel_format = PixelFormat::NV11; }
        else if(sub_type == MFVideoFormat_NV12) { format.pixel_format = PixelFormat::NV12; }
        else if(sub_type == MFVideoFormat_YV12) { format.pixel_format = PixelFormat::YV12; }
        else if(sub_type == MFVideoFormat_I420) { format.pixel_format = PixelFormat::I420; }
        else if(sub_type == MFVideoFormat_IYUV) { format.pixel_format = PixelFormat::IYUV; }
        else if(sub_type == MFVideoFormat_Y210) { format.pixel_format = PixelFormat::Y210; }
        else if(sub_type == MFVideoFormat_Y216) { format.pixel_format = PixelFormat::Y216; }
        else if(sub_type == MFVideoFormat_Y410) { format.pixel_format = PixelFormat::Y410; }
        else if(sub_type == MFVideoFormat_Y416) { format.pixel_format = PixelFormat::Y416; }
        else if(sub_type == MFVideoFormat_Y41P) { format.pixel_format = PixelFormat::Y41P; }
        else if(sub_type == MFVideoFormat_Y41T) { format.pixel_format = PixelFormat::Y41T; }
        else if(sub_type == MFVideoFormat_Y42T) { format.pixel_format = PixelFormat::Y42T; }
        else if(sub_type == MFVideoFormat_P210) { format.pixel_format = PixelFormat::P210; }
        else if(sub_type == MFVideoFormat_P216) { format.pixel_format = PixelFormat::P216; }
        else if(sub_type == MFVideoFormat_P010) { format.pixel_format = PixelFormat::P010; }
        else if(sub_type == MFVideoFormat_P016) { format.pixel_format = PixelFormat::P016; }
        else if(sub_type == MFVideoFormat_v210) { format.pixel_format = PixelFormat::v210; }
        else if(sub_type == MFVideoFormat_v216) { format.pixel_format = PixelFormat::v216; }
        else if(sub_type == MFVideoFormat_v410) { format.pixel_format = PixelFormat::v410; }
        else if(sub_type == MFVideoFormat_MP43) { format.pixel_format = PixelFormat::MP43; }
        else if(sub_type == MFVideoFormat_MP4S) { format.pixel_format = PixelFormat::MP4S; }
        else if(sub_type == MFVideoFormat_M4S2) { format.pixel_format = PixelFormat::M4S2; }
        else if(sub_type == MFVideoFormat_MP4V) { format.pixel_format = PixelFormat::MP4V; }
        else if(sub_type == MFVideoFormat_WMV1) { format.pixel_format = PixelFormat::WMV1; }
        else if(sub_type == MFVideoFormat_WMV2) { format.pixel_format = PixelFormat::WMV2; }
        else if(sub_type == MFVideoFormat_WMV3) { format.pixel_format = PixelFormat::WMV3; }
        else if(sub_type == MFVideoFormat_WVC1) { format.pixel_format = PixelFormat::WVC1; }
        else if(sub_type == MFVideoFormat_MSS1) { format.pixel_format = PixelFormat::MSS1; }
        else if(sub_type == MFVideoFormat_MSS2) { format.pixel_format = PixelFormat::MSS2; }
        else if(sub_type == MFVideoFormat_MPG1) { format.pixel_format = PixelFormat::MPG1; }
        else if(sub_type == MFVideoFormat_DVSL) { format.pixel_format = PixelFormat::DVSL; }
        else if(sub_type == MFVideoFormat_DVSD) { format.pixel_format = PixelFormat::DVSD; }
        else if(sub_type == MFVideoFormat_DVHD) { format.pixel_format = PixelFormat::DVHD; }
        else if(sub_type == MFVideoFormat_DV25) { format.pixel_format = PixelFormat::DV25; }
        else if(sub_type == MFVideoFormat_DV50) { format.pixel_format = PixelFormat::DV50; }
        else if(sub_type == MFVideoFormat_DVH1) { format.pixel_format = PixelFormat::DVH1; }
        else if(sub_type == MFVideoFormat_DVC)  { format.pixel_format = PixelFormat::DVC; }
        else if(sub_type == MFVideoFormat_H264) { format.pixel_format = PixelFormat::H264; }
        else if(sub_type == MFVideoFormat_H265) { format.pixel_format = PixelFormat::H265; }
        else if(sub_type == MFVideoFormat_MJPG) { format.pixel_format = PixelFormat::MJPG; }
        else if(sub_type == MFVideoFormat_420O) { format.pixel_format = PixelFormat::_420O; }
        else if(sub_type == MFVideoFormat_HEVC) { format.pixel_format = PixelFormat::HEVC; }
        else if(sub_type == MFVideoFormat_HEVC_ES) { format.pixel_format = PixelFormat::HEVC_ES; }
        else if(sub_type == MFVideoFormat_VP80)    { format.pixel_format = PixelFormat::VP80; }
        else if(sub_type == MFVideoFormat_VP90)    { format.pixel_format = PixelFormat::VP90; }
        else if(sub_type == MFVideoFormat_ORAW)    { format.pixel_format = PixelFormat::ORAW; }
#if (WINVER >= _WIN32_WINNT_WIN8)
        else if (sub_type == MFVideoFormat_H263) { format.pixel_format = PixelFormat::H263; }
#endif // (WINVER >= _WIN32_WINNT_WIN8)

#if (WDK_NTDDI_VERSION >= NTDDI_WIN10)
        else if (sub_type == MFVideoFormat_A2R10G10B10) { format.pixel_format = PixelFormat::A2R10G10B10; }
        else if (sub_type == MFVideoFormat_A16B16G16R16F) { format.pixel_format = PixelFormat::A16B16G16R16F; }
#endif

#if (WDK_NTDDI_VERSION >= NTDDI_WIN10_RS3)
        else if (sub_type == MFVideoFormat_VP10) { format.pixel_format = PixelFormat::VP10; }
        else if (sub_type == MFVideoFormat_AV1) { format.pixel_format = PixelFormat::AV1; }
#endif

        release(media_type);
        result.success = true;
        return result;
    } 
}


#include "camera_usb.hpp"
#include "mjpeg_convert.hpp"


namespace camera_usb
{
    CameraList enumerate_cameras();

    void close(CameraList& cameras);

    bool open_camera(Camera& camera);

    void grab_image(Camera& camera, img::ImageView const& dst);

    void stream_camera(Camera& camera, grab_cb const& on_grab, bool_fn const& stream_condition);
}


#define MJPEG_CONVERT_IMPLEMENTATION
#include "mjpeg_convert.hpp"