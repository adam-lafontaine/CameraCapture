 #pragma once

#include "image.hpp"


/* pixel format */

namespace convert
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
        YUYV = fcc_to_u32("YUYV"),
        YUNV = fcc_to_u32("YUNV"),
        YUY2 = fcc_to_u32("YUY2"),
        YVYU = fcc_to_u32("YVYU"),
        YVU9 = fcc_to_u32("YVU9"),
        UYVY = fcc_to_u32("UYVY"),
        Y422 = fcc_to_u32("Y422"),
        UYNV = fcc_to_u32("UYNV"),
        HDYC = fcc_to_u32("HDYC"),
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

        Unknown = fcc_to_u32("XXXX"),
        Invalid = 0
    };


    static inline void u32_to_fcc(u32 bytes4, char* dst)
    {
        union
        {
            u32 value;
            char fcc[4];
        } sd;

        sd.value = bytes4;

        for (u32 i = 0; i < 4; i++)
        {
            dst[i] = sd.fcc[i];
        }
    }


    static inline void pf_to_fcc(PixelFormat pf, char* dst)
    {
        u32_to_fcc((u32)pf, dst);
    }


    static inline PixelFormat fcc_to_pf(cstr fcc)
    {
        return (PixelFormat)fcc_to_u32(fcc);
    }
}


namespace convert
{
    namespace img = image;

    PixelFormat validate_format(u32 src_len, u32 width, u32 height, PixelFormat format);

    void convert_view(SpanView<u8> const& src, img::ImageView const& dst, PixelFormat format);

    void convert_sub_view(SpanView<u8> const& src, img::SubView const& dst, PixelFormat format);


    using ViewYUV = img::View3<u8>;

    ViewYUV make_view_yuv(u32 width, u32 height, img::Buffer8& buffer);


    void to_yuv(SpanView<u8> const& src, u32 width, u32 height, ViewYUV const& dst, PixelFormat format);


    void yuv_to_rgba(ViewYUV const& src, img::ImageView const& dst);

    void yuv_to_rgba(ViewYUV const& src, img::SubView const& dst);
}