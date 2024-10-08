#pragma once

#include "convert.hpp"

/* yuyv_to_planar */

namespace convert
{
    class OffsetYUYV
    {
    public:
        u8 y1;
        u8 y2;
        u8 u;
        u8 v;
    };


    class OffsetUV
    {
    public:
        u8 u;
        u8 v;
    };


    constexpr OffsetYUYV offset_yuyv(PixelFormat pf)
    {
        using PF = PixelFormat;

        OffsetYUYV yuyv{};

        switch (pf)
        {
        case PF::YUYV:
        case PF::YUNV:
        case PF::YUY2:            
            yuyv.y1 = 0;
            yuyv.u  = 1;
            yuyv.y2 = 2;
            yuyv.v  = 3;
            break;
        
        case PF::YVYU:
            yuyv.y1 = 0;
            yuyv.v  = 1;
            yuyv.y2 = 2;
            yuyv.u  = 3;
            break;
        
        case PF::UYVY:
        case PF::Y422:
        case PF::UYNV:
        case PF::HDYC:
            yuyv.u  = 0;
            yuyv.y1 = 1;
            yuyv.v  = 2;
            yuyv.y2 = 3;
            break;

        default:
            yuyv.y1 = 0;
            yuyv.y2 = 0;
            yuyv.u  = 0;
            yuyv.v  = 0;
            break;
        }

        return yuyv;
    }


    constexpr OffsetUV offset_uv(PixelFormat pf)
    {
        using PF = PixelFormat;

        OffsetUV uv{};

        switch (pf)
        {
        case PF::NV12:
        case PF::P010:
            uv.u = 0;
            uv.v = 1;
            break;

        case PF::NV21:
            uv.u = 1;
            uv.v = 0;
        
        default:
            uv.u = 0;
            uv.v = 0;
            break;
        }

        return uv;
    }
    

    static void yuyv_to_planar_1_1(img::View1<u32> const& src, ViewYUV const& dst, OffsetYUYV yuyv)
    {
        assert(src.width == dst.width / 2);
        assert(src.height == dst.height);

        auto s = (u8*)src.matrix_data_;

        auto sy1 = s + yuyv.y1;
        auto sy2 = s + yuyv.y2;
        auto su = s + yuyv.u;
        auto sv = s + yuyv.v;

        auto dy = dst.channel_data[(int)YUV::Y];
        auto du = dst.channel_data[(int)YUV::U];
        auto dv = dst.channel_data[(int)YUV::V];

        auto len = src.width * src.height;

        for (u32 i = 0; i < len; i++)
        {
            auto si = 4 * i;
            auto di = 2 * i;

            dy[di] = sy1[si];
            dy[di + 1] = sy2[si];

            du[di] = du[di + 1] = su[si];
            dv[di] = dv[di + 1] = sv[si];
        }
    }


    static void yuyv_to_planar_2_1(img::View1<u32> const& src, ViewYUV const& dst, OffsetYUYV yuyv)
    {
        assert(src.width == dst.width);
        assert(src.height == 2 * dst.height);

        auto src_pitch = src.width * sizeof(u32);
        auto p2 = 2 * src_pitch;

        auto s = (u8*)src.matrix_data_;

        auto sy1 = s + yuyv.y1;
        auto sy2 = s + yuyv.y2;
        auto sy3 = sy1 + src_pitch;
        auto sy4 = sy2 + src_pitch;

        auto su1 = s + yuyv.u;
        auto su2 = su1 + src_pitch;

        auto sv1 = s + yuyv.v;
        auto sv2 = sv1 + src_pitch;

        auto dy = dst.channel_data[(int)YUV::Y];
        auto du = dst.channel_data[(int)YUV::U];
        auto dv = dst.channel_data[(int)YUV::V];

        u32 i = 0;
        for (u32 h = 0; h < dst.height; h++)
        {
            for (u32 w = 0; w < dst.width; w++)
            {
                auto sw = 4 * w;
                auto y = ((u32)sy1[sw] + sy2[sw] + sy3[sw] + sy4[sw]) / 4;
                auto u = ((u32)su1[sw] + su2[sw]) / 2;
                auto v = ((u32)sv1[sw] + sv2[sw]) / 2;

                dy[i] = (u8)y;
                du[i] = (u8)u;
                dv[i] = (u8)v;

                ++i;
            }

            sy1 += p2;
            sy2 += p2;
            sy3 += p2;
            sy4 += p2;
            su1 += p2;
            su2 += p2;
            sv1 += p2;
            sv2 += p2;
        }
    }
    

    static void nv12_to_planar_1_1(img::View1u8 const& src_y, img::View1<u16> const& src_uv, ViewYUV const& dst, OffsetUV uv)
    { 
        assert(src_y.width == dst.width);
        assert(src_y.height == dst.height);
        assert(src_uv.width == dst.width / 2);
        assert(src_uv.height == dst.height / 2);

        auto width = dst.width;
        auto height = dst.height;
        auto len = width * height;

        img::copy(src_y, img::select_channel(dst, (u32)YUV::Y));

        auto suv = (u8*)src_uv.matrix_data_;
        auto su = suv + uv.u;
        auto sv = suv + uv.v;

        auto du = dst.channel_data[(u32)YUV::U];
        auto dv = dst.channel_data[(u32)YUV::V];

        for (u32 h = 0; h < height; h += 2)
        {
            auto du1 = du + h * width;
            auto du2 = du1 + 1;
            auto du3 = du1 + width;
            auto du4 = du3 + 1;

            auto dv1 = dv + h * width;
            auto dv2 = dv1 + 1;
            auto dv3 = dv1 + width;
            auto dv4 = dv3 + 1;

            for (u32 w = 0; w < width; w += 2)
            {  
                *du1 = *du2 = *du3 = *du4 = *su;
                *dv1 = *dv2 = *dv3 = *dv4 = *sv;
                
                su += 2;
                sv += 2;

                du1 += 2;
                du2 += 2;
                du3 += 2;
                du4 += 2;

                dv1 += 2;
                dv2 += 2;
                dv3 += 2;
                dv4 += 2;
            }
        }
    }


    static void nv12_to_planar_2_1(img::View1u8 const& src_y, img::View1<u16> const& src_uv, ViewYUV const& dst, OffsetUV uv)
    {
        assert(src_y.width == dst.width * 2);
        assert(src_y.height == dst.height * 2);
        assert(src_uv.width == dst.width);
        assert(src_uv.height == dst.height);

        auto width = dst.width;
        auto height = dst.height;
        
        auto py = 2 * src_y.width;        

        auto sy1 = src_y.matrix_data_;
        auto sy2 = sy1 + 1;
        auto sy3 = sy1 + src_y.width;
        auto sy4 = sy3 + 1;

        auto suv = (u8*)src_uv.matrix_data_;
        auto su = suv + uv.u;
        auto sv = suv + uv.v;

        auto dy = dst.channel_data[(u32)YUV::Y];
        auto du = dst.channel_data[(u32)YUV::U];
        auto dv = dst.channel_data[(u32)YUV::V];

        u32 i = 0;
        for (u32 h = 0; h < height; h++)
        {
            for (u32 w = 0; w < width; w++)
            {
                auto sw = 2 * w;
                auto y = ((u32)sy1[sw] + sy2[sw] + sy3[sw] + sy4[sw]) / 4;
                dy[i] = (u8)y;

                du[i] = *su;
                dv[i] = *sv;

                ++i;
                ++su;
                ++sv;
            }

            sy1 += py;
            sy2 += py;
            sy3 += py;
            sy4 += py;
        }
    }
    

    static void yv12_to_planar_1_1(img::View1u8 const& src_y, img::View1u8 const& src_u, img::View1u8 const& src_v, ViewYUV const& dst)
    { 
        assert(src_y.width == dst.width);
        assert(src_y.height == dst.height);
        assert(src_u.width == dst.width / 2);
        assert(src_u.height == dst.height / 2);
        assert(src_v.width == dst.width / 2);
        assert(src_v.height == dst.height / 2);

        img::copy(src_y, img::select_channel(dst, (u32)YUV::Y));

        auto width = dst.width;
        auto height = dst.height;
        
        auto su = src_u.matrix_data_;
        auto sv = src_v.matrix_data_;

        auto du = dst.channel_data[(u32)YUV::U];
        auto dv = dst.channel_data[(u32)YUV::V];

        for (u32 h = 0; h < height; h += 2)
        {
            auto du1 = du + h * width;
            auto du2 = du1 + 1;
            auto du3 = du1 + width;
            auto du4 = du3 + 1;

            auto dv1 = dv + h * width;
            auto dv2 = dv1 + 1;
            auto dv3 = dv1 + width;
            auto dv4 = dv3 + 1;

            for (u32 w = 0; w < width; w += 2)
            {  
                *du1 = *du2 = *du3 = *du4 = *su;
                *dv1 = *dv2 = *dv3 = *dv4 = *sv;
                
                su += 1;
                sv += 1;

                du1 += 2;
                du2 += 2;
                du3 += 2;
                du4 += 2;

                dv1 += 2;
                dv2 += 2;
                dv3 += 2;
                dv4 += 2;
            }
        }
    }


    static void yv12_to_planar_2_1(img::View1u8 const& src_y, img::View1u8 const& src_u, img::View1u8 const& src_v, ViewYUV const& dst)
    {
        assert(src_y.width == dst.width * 2);
        assert(src_y.height == dst.height * 2);
        assert(src_u.width == dst.width);
        assert(src_u.height == dst.height);
        assert(src_v.width == dst.width);
        assert(src_v.height == dst.height);

        img::copy(src_u, img::select_channel(dst, (u32)YUV::U));
        img::copy(src_y, img::select_channel(dst, (u32)YUV::Y));

        auto width = dst.width;
        auto height = dst.height;

        auto src_pitch = src_y.width;
        auto p2 = 2 * src_pitch;        

        auto sy1 = src_y.matrix_data_;
        auto sy2 = sy1 + 1;
        auto sy3 = sy1 + src_pitch;
        auto sy4 = sy3 + 1;

        auto dy = dst.channel_data[(u32)YUV::Y];

        u32 i = 0;
        for (u32 h = 0; h < height; h++)
        {
            for (u32 w = 0; w < width; w++)
            {
                auto sw = 2 * w;
                auto y = ((u32)sy1[sw] + sy2[sw] + sy3[sw] + sy4[sw]) / 4;
                dy[i] = (u8)y;
                ++i;
            }

            sy1 += p2;
            sy2 += p2;
            sy3 += p2;
            sy4 += p2;
        }
    }
}


/* yuv to rgb */

namespace convert
{
    namespace yuv
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

        constexpr f32 f = 1.0f / 255.0f;
    }   


    template <typename T>
    static void yuv_to_rgb(T y, T u, T v, u8* r, u8* g, u8* b)
    {
        auto yf = y;
        auto uf = u - 127.5f;
        auto vf = v - 127.5f;

        auto rf = (yuv::yr * yf) + (yuv::ur * uf) + (yuv::vr * vf);
        auto gf = (yuv::yg * yf) + (yuv::ug * uf) + (yuv::vg * vf);
        auto bf = (yuv::yb * yf) + (yuv::ub * uf) + (yuv::vb * vf);

        *r = num::round_to_unsigned<u8>(num::clamp(rf, 0.0f, 255.0f));
        *g = num::round_to_unsigned<u8>(num::clamp(gf, 0.0f, 255.0f));
        *b = num::round_to_unsigned<u8>(num::clamp(bf, 0.0f, 255.0f));
    }


    template <typename T>
    static void yuv_to_rgb(T y, T u, T v, img::Pixel* dst)
    {   
        auto yf = y;
        auto uf = u - 127.5f;
        auto vf = v - 127.5f;

        auto rf = (yuv::yr * yf) + (yuv::ur * uf) + (yuv::vr * vf);
        auto gf = (yuv::yg * yf) + (yuv::ug * uf) + (yuv::vg * vf);
        auto bf = (yuv::yb * yf) + (yuv::ub * uf) + (yuv::vb * vf);

        dst->red = num::round_to_unsigned<u8>(num::clamp(rf, 0.0f, 255.0f));
        dst->green = num::round_to_unsigned<u8>(num::clamp(gf, 0.0f, 255.0f));
        dst->blue = num::round_to_unsigned<u8>(num::clamp(bf, 0.0f, 255.0f));
    }


    template <typename T>
    static void yuv_to_rgba(T* y, T* u, T* v, img::Pixel* dst, u32 len)
    {
        constexpr auto S = sizeof(img::Pixel);

        auto r = &dst->red;
        auto g = &dst->green;
        auto b = &dst->blue;
        auto a = &dst->alpha;

        for (u32 i = 0; i < len; i++)
        {
            yuv_to_rgb(y[i], u[i], v[i], r, g, b);
            *a = 255;

            r += S;
            g += S;
            b += S;
            a += S;
        }
    }


    template <typename T>
    static void yuv_to_rgb(T* y, T* u, T* v, u8* r, u8* g, u8* b, u32 len)
    {
        for (u32 i = 0; i < len; i++)
        {
            yuv_to_rgb(y[i], u[i], v[i], r, g, b);
            ++r;
            ++g;
            ++b;
        }
    }
}


/* to rgba */

namespace convert
{
    static void span_yuyv_to_pixel(SpanView<u8> const& src, SpanView<img::Pixel> const& dst, OffsetYUYV yuyv)
    {
        auto const len  = src.length;

        auto s = src.begin;

        auto d1 = dst.begin;
        auto d2 = d1 + 1;

        auto y1 = yuyv.y1;
        auto y2 = yuyv.y2;
        auto u = yuyv.u;
        auto v = yuyv.v;

        for (u32 i = 0; i < len; i += 4)
        {
            yuv_to_rgb(s[y1], s[u], s[v], d1);
            yuv_to_rgb(s[y2], s[u], s[v], d2);

            d1->alpha = 255;
            d2->alpha = 255;

            d1 += 2;
            d2 += 2;

            s += 4;
        }
    }


    static void yuyv_to_rgba(SpanView<u8> const& src, img::ImageView const& dst, PixelFormat format)
    {
        assert(src.length == dst.width * dst.height * 2);

        auto yuyv = offset_yuyv(format);

        span_yuyv_to_pixel(src, img::to_span(dst), yuyv);
    }


    static void yuyv_to_rgba(SpanView<u8> const& src, img::SubView const& dst, PixelFormat format)
    {
        assert(src.length == dst.width * dst.height * 2);

        auto yuyv = offset_yuyv(format);

        auto const w = dst.width;

        for (u32 y = 0; y < dst.height; y++)
        {
            span_yuyv_to_pixel(span::sub_view(src, y * w, w), img::row_span(dst, y), yuyv);
        }
    }
    

    template <class VIEW>
    static void nv12_to_rgba(SpanView<u8> const& src, VIEW const& dst, PixelFormat format)
    {
        auto const width = dst.width;
        auto const height = dst.height;

        assert(src.length == width * height + width * height / 2);

        auto sy = src.begin;
        auto suv = sy + width * height;
        auto sd = img::row_begin(dst, 0);

        auto uv = offset_uv(format);

        auto u = suv + uv.u;
        auto v = suv + uv.v;        

        for (u32 h = 0; h < height; h += 2)
        {
            auto y1 = sy + h * width;
            auto y2 = y1 + 1;
            auto y3 = y1 + width;
            auto y4 = y3 + 1;

            auto d1 = img::row_begin(dst, h);
            auto d2 = d1 + 1;
            auto d3 = img::row_begin(dst, h + 1);
            auto d4 = d3 + 1;

            for (u32 w = 0; w < width; w += 2)
            {  
                yuv_to_rgb(*y1, *u, *v, d1);
                yuv_to_rgb(*y2, *u, *v, d2);
                yuv_to_rgb(*y3, *u, *v, d3);
                yuv_to_rgb(*y4, *u, *v, d4);

                d1->alpha = 255;
                d2->alpha = 255;
                d3->alpha = 255;
                d4->alpha = 255;

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
    
    
    template <class VIEW>
    static void yv12_to_rgba(SpanView<u8> const& src, VIEW const& dst, PixelFormat format)
    {
        auto const width = dst.width;
        auto const height = dst.height;

        assert(src.length == width * height + width * height / 2);

        auto sy = src.begin;
        auto suv = sy + width * height;
        auto sd = img::row_begin(dst, 0);

        auto u = suv;
        auto v = suv;

        using PF = PixelFormat;

        switch (format)
        {
        case PF::YV12:
            u += width * height / 4;
            break;

        case PF::I420:
        case PF::IYUV:
            v += width * height / 4;
            break;
        }

        for (u32 h = 0; h < height; h += 2)
        {
            auto y1 = sy + h * width;
            auto y2 = y1 + 1;
            auto y3 = y1 + width;
            auto y4 = y3 + 1;

            auto d1 = img::row_begin(dst, h);
            auto d2 = d1 + 1;
            auto d3 = img::row_begin(dst, h + 1);
            auto d4 = d3 + 1;

            for (u32 w = 0; w < width; w += 2)
            {  
                yuv_to_rgb(*y1, *u, *v, d1);
                yuv_to_rgb(*y2, *u, *v, d2);
                yuv_to_rgb(*y3, *u, *v, d3);
                yuv_to_rgb(*y4, *u, *v, d4);

                d1->alpha = 255;
                d2->alpha = 255;
                d3->alpha = 255;
                d4->alpha = 255;

                y1 += 2;
                y2 += 2;
                y3 += 2;
                y4 += 2;

                d1 += 2;
                d2 += 2;
                d3 += 2;
                d4 += 2;

                u += 1;
                v += 1;
            }
        }
    }
}


/* to yuv planar */

namespace convert
{
    static void yuyv_to_yuv(SpanView<u8> const& src, u32 width, u32 height, ViewYUV const& dst, PixelFormat format)
    {
        assert(src.length == width * height * 2);

        auto yuyv = offset_yuyv(format);

        img::View1<u32> v32{};
        v32.width = width / 2;
        v32.height = height;
        v32.matrix_data_ = (u32*)src.begin;

        if (height == dst.height)
        {
            yuyv_to_planar_1_1(v32, dst, yuyv);
        }
        else
        {
           yuyv_to_planar_2_1(v32, dst, yuyv);
        }
    }


    static void nv12_to_yuv(SpanView<u8> const& src, u32 width, u32 height, ViewYUV const& dst, PixelFormat format)
    { 
        //                  |--- nv12 y ---| |------- nv12 uv ------|
        assert(src.length == width * height + 2 * width * height / 4);

        img::View1u8 src_y{};
        src_y.width = width;
        src_y.height = height;
        src_y.matrix_data_ = src.begin;

        img::View1<u16> src_uv{};
        src_uv.width = width / 2;
        src_uv.height = height / 2;
        src_uv.matrix_data_ = (u16*)(src.begin + width * height);

        auto uv = offset_uv(format);

        if (height == dst.height)
        {
            nv12_to_planar_1_1(src_y, src_uv, dst, uv);
        }
        else
        {
            nv12_to_planar_2_1(src_y, src_uv, dst, uv);
        }
    }


    static void yv12_to_yuv(SpanView<u8> const& src, u32 width, u32 height, ViewYUV const& dst, PixelFormat format)
    { 
        //                  |--- yv12 y ---| |--------- yv12 u --------| |--------- yv12 v --------|
        assert(src.length == width * height + (width / 2) * (width / 2) + (width / 2) * (width / 2) );

        img::View1u8 src_y{};
        src_y.width = width;
        src_y.height = height;
        src_y.matrix_data_ = src.begin;

        img::View1u8 src_u{};
        src_u.width = width / 2;
        src_u.height = height / 2;
        src_u.matrix_data_ = src.begin + width * height;

        auto src_v = src_u;

        using PF = PixelFormat;

        switch (format)
        {
        case PF::YV12:
            src_u.matrix_data_ += width * height / 4;
            break;

        case PF::I420:
        case PF::IYUV:
            src_v.matrix_data_ += width * height / 4;
            break;
        }

        if (height == dst.height)
        {
            yv12_to_planar_1_1(src_y, src_u, src_v, dst);
        }
        else
        {
            yv12_to_planar_2_1(src_y, src_u, src_v, dst);
        }
    }
}


/* api */

namespace convert
{
    PixelFormat validate_format(u32 src_len, u32 width, u32 height, PixelFormat format)
    {
        using PF = PixelFormat;

        u32 is_valid = 0;

        switch (format)
        {
        case PF::YUYV:
        case PF::YUNV:
        case PF::YUY2:
        case PF::YVYU:
        case PF::UYVY:
        case PF::Y422:
        case PF::UYNV:
        case PF::HDYC:
            is_valid = src_len == width * height * 2;
            break;
        
        case PF::NV12:
        case PF::NV21:
        case PF::P010:
        case PF::YV12:
        case PF::I420:
        case PF::IYUV:
            is_valid = src_len == width * height + width * height / 2;
            break;

        default:
            break;
        }

        return (PF)(is_valid * (u32)format);
    }
    
    
    void convert_view(SpanView<u8> const& src, img::ImageView const& dst, PixelFormat format)
    {
        using PF = PixelFormat;

        switch (format)
        {
        case PF::YUYV:
        case PF::YUNV:
        case PF::YUY2:
        case PF::YVYU:
        case PF::UYVY:
        case PF::Y422:
        case PF::UYNV:
        case PF::HDYC:
            yuyv_to_rgba(src, dst, format);
            break;

        case PF::NV12:
        case PF::NV21:
        case PF::P010:
            nv12_to_rgba(src, dst, format);
            break;
        
        case PF::YV12:
        case PF::I420:
        case PF::IYUV:
            yv12_to_rgba(src, dst, format);
            break;

        default:
            img::fill(dst, img::to_pixel(100));
        }
    }


    void convert_sub_view(SpanView<u8> const& src, img::SubView const& dst, PixelFormat format)
    {
        using PF = PixelFormat;

        switch (format)
        {
        case PF::YUYV:
        case PF::YUNV:
        case PF::YUY2:
        case PF::YVYU:
        case PF::UYVY:
        case PF::Y422:
        case PF::UYNV:
        case PF::HDYC:
            yuyv_to_rgba(src, dst, format);
            break;

        case PF::NV12:
        case PF::NV21:
        case PF::P010:
            nv12_to_rgba(src, dst, format);
            break;
        
        case PF::YV12:
        case PF::I420:
        case PF::IYUV:
            yv12_to_rgba(src, dst, format);
            break;

        default:
            img::fill(dst, img::to_pixel(100));
        }
    }


    void to_yuv(SpanView<u8> const& src, u32 width, u32 height, ViewYUV const& dst, PixelFormat format)
    {
        using PF = PixelFormat;

        switch (format)
        {
        case PF::YUYV:
        case PF::YUNV:
        case PF::YUY2:
        case PF::YVYU:
        case PF::UYVY:
        case PF::Y422:
        case PF::UYNV:
        case PF::HDYC:
            yuyv_to_yuv(src, width, height, dst, format);
            break;

        case PF::NV12:
        case PF::NV21:
        case PF::P010:
            nv12_to_yuv(src, width, height, dst, format);
            break;

        case PF::YV12:
        case PF::I420:
        case PF::IYUV:
            yv12_to_yuv(src, width, height, dst, format);
            break;
        }
    }


    void yuv_to_rgba(ViewYUV const& src, img::ImageView const& dst)
    {
        assert(src.width == dst.width);
        assert(src.height == dst.height);

        auto len = src.width * src.height;

        auto y = src.channel_data[(u32)YUV::Y];
        auto u = src.channel_data[(u32)YUV::U];
        auto v = src.channel_data[(u32)YUV::V];

        auto d = dst.matrix_data_;

        yuv_to_rgba(y, u, v, d, len);
    }


    void yuv_to_rgba(ViewYUV const& src, img::SubView const& dst)
    {
        assert(src.width == dst.width);
        assert(src.height == dst.height);

        auto len = src.width;

        auto y = src.channel_data[(u32)YUV::Y];
        auto u = src.channel_data[(u32)YUV::U];
        auto v = src.channel_data[(u32)YUV::V];

        for (u32 h = 0; h < dst.height; y++)
        {
            auto d = img::row_begin(dst, h);
            yuv_to_rgba(y, u, v, d, len);

            y += len;
            u += len;
            v += len;
        }
    }


    void yuv_to_rgb(ViewYUV const& src, img::ViewRGBu8 const& dst)
    {
        assert(src.width == dst.width);
        assert(src.height == dst.height);

        auto len = src.width * src.height;

        auto y = src.channel_data[(u32)YUV::Y];
        auto u = src.channel_data[(u32)YUV::U];
        auto v = src.channel_data[(u32)YUV::V];

        auto r = dst.channel_data[(u32)img::RGB::R];
        auto g = dst.channel_data[(u32)img::RGB::G];
        auto b = dst.channel_data[(u32)img::RGB::B];

        yuv_to_rgb(y, u, v, r, g, b, len);
    }
}


