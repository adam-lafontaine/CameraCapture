#pragma once

#include "convert.hpp"

/* yuyv_to_planar */

namespace convert
{
    enum class YUV : u32 { Y = 0, U = 1, V = 2 };


    class OffsetYUYV
    {
    public:
        u8 y1;
        u8 y2;
        u8 u;
        u8 v;
    };


    struct UV
    {
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

        }

        return yuyv;
    }
    

    static void yuyv_to_planar(img::View1<u32> const& src, ViewYUV const& dst, OffsetYUYV yuyv)
    {
        assert(src.width == dst.width);
        assert(src.height == dst.height);

        auto const len = src.width * src.height * 2;

        auto s = (u8*)src.matrix_data_;

        auto y1 = yuyv.y1;
        auto y2 = yuyv.y2;
        auto u = yuyv.u;
        auto v = yuyv.v;

        auto dy1 = dst.channel_data[(int)YUV::Y];
        auto du1 = dst.channel_data[(int)YUV::U];
        auto dv1 = dst.channel_data[(int)YUV::V];

        auto dy2 = dy1 + 1;
        auto du2 = du1 + 1;
        auto dv2 = dv1 + 1;

        for (u32 i = 0; i < len; i += 4)
        {
            *dy1 = s[y1];            
            *dy2 = s[y2];

            *du1 = *du2 = s[u];
            *dv1 = *dv2 = s[v];

            dy1 += 2;
            du1 += 2;
            dv1 += 2;

            dy2 += 2;
            du2 += 2;
            dv2 += 2;

            s += 4;
        }
    }
    

    static void nv12_to_planar(img::View1u8 const& src_y, img::View1<UV> const& src_uv, ViewYUV const& dst)
    { 
        auto width = dst.width;
        auto height = dst.height;
        auto len = width * height;

        img::copy(src_y, img::select_channel(dst, (u32)YUV::Y));

        auto suv = src_uv.matrix_data_;
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
                *du1 = *du2 = *du3 = *du4 = suv->u;
                *dv1 = *dv2 = *dv3 = *dv4 = suv->v;
                
                suv++;

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
}


/* yuv to planar scaled, not tested */

namespace convert
{
    static void yuyv_to_planar(img::View1<u32> const& src, ViewYUV const& dst, OffsetYUYV yuyv, u32 scale)
    {
        auto y1 = yuyv.y1;
        auto y2 = yuyv.y2;
        auto u = yuyv.u;
        auto v = yuyv.v;

        assert(scale == src.height / dst.height);

        u32 const N = scale;

        constexpr u32 N_MAX = 8;

        assert(N >= 2 || N % 2 == 0);
        assert(N <= N_MAX);

        u32 N2 = N / 2;
        u32 NN = N * N;
        f32 iNN = 1.0f / (NN);

        auto dy = img::select_channel(dst, (u32)YUV::Y);
        auto du = img::select_channel(dst, (u32)YUV::U);
        auto dv = img::select_channel(dst, (u32)YUV::V);

        auto srect = img::make_rect(N2, N);

        f32 yf = 0.0f;
        f32 uf = 0.0f;
        f32 vf = 0.0f;
        
        for (u32 h = 0; h < dst.height; h++)
        {
            auto drow_y = img::row_begin(dy, h);
            auto drow_u = img::row_begin(du, h);
            auto drow_v = img::row_begin(dv, h);

            for (u32 w = 0; w < dst.width; w++)
            {
                yf = uf = vf = 0.0f;

                auto sub = img::sub_view(src, srect);
                for (u32 h = 0; h < sub.height; h++)
                {
                    auto srow = img::row_begin(sub, h);
                    for (u32 w = 0; w < sub.width; w++)
                    {
                        auto s8 = (u8*)(srow + w);
                        yf += s8[y1] + s8[y2];
                        uf += s8[u];
                        vf += s8[v];
                    }
                }

                drow_y[w] = yf * iNN;
                drow_u[w] = uf * 2 * iNN;
                drow_v[w] = vf * 2 * iNN;

                srect.x_begin += N2;
                srect.x_end += N2;
            }

            srect.y_begin += N;
            srect.y_end += N;
        }
    }


    static void nv12_to_planar(img::View1u8 const& src_y, img::View1<UV> const& src_uv, ViewYUV const& dst, u32 scale)
    {
        assert(scale == src_y.height / dst.height);

        auto sw = src_y.width;
        auto sh = src_y.height;

        auto dw = dst.width;
        auto dh = dst.height;        

        u32 const N = scale;

        constexpr u32 N_MAX = 8;

        assert(N >= 2 || N % 2 == 0);
        assert(N <= N_MAX);

        u32 const TN = 2 * N;
        
        f32 const iNN = 1.0f / (N * N);

        auto dy = img::select_channel(dst, (u32)YUV::Y);

        auto srect_y = img::make_rect(N, N);

        f32 yf = 0.0f;
        f32 uf = 0.0f;
        f32 vf = 0.0f;

        for (u32 h = 0; h < dh; h++)
        {
            auto drow_y = img::row_begin(dy, h);

            for (u32 w = 0; w < dw; w++)
            {
                yf = 0.0f;
                auto sub_y = img::sub_view(src_y, srect_y);
                for (u32 hs = 0; hs < sub_y.height; hs++)
                {
                    auto srow = img::row_begin(sub_y, hs);
                    for (u32 ws = 0; ws < sub_y.width; ws++)
                    {
                        yf += srow[ws];
                    }
                }

                drow_y[w] = yf * iNN;

                srect_y.x_begin += N;
                srect_y.x_end += N;
            }

            srect_y.y_begin += N;
            srect_y.y_end += N;
        }

        auto suv = src_uv.matrix_data_;

        auto du = dst.channel_data[(u32)YUV::U];
        auto dv = dst.channel_data[(u32)YUV::V];

        auto len = dw * dh;

        for (u32 i = 0; i < len; i++)
        {
            auto uv = suv[i];
            du[i] = uv.u;
            dv[i] = uv.v;
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
}


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


    static inline void span_yuyv_to_view(SpanView<u8> const& src, img::ImageView const& dst, OffsetYUYV yuyv)
    {
        span_yuyv_to_pixel(src, img::to_span(dst), yuyv);
    }


    static inline void span_yuyv_to_sub_view(SpanView<u8> const& src, img::SubView const& dst, OffsetYUYV yuyv)
    {
        auto const w = dst.width;

        for (u32 y = 0; y < dst.height; y++)
        {
            span_yuyv_to_pixel(span::sub_view(src, y * w, w), img::row_span(dst, y), yuyv);
        }
    }


    
}


/* to rgba */

namespace convert
{
    void yuyv_to_rgba(SpanView<u8> const& src, img::ImageView const& dst, PixelFormat format)
    {
        assert(src.length == dst.width * dst.height * 2);

        auto yuyv = offset_yuyv(format);
        span_yuyv_to_view(src, dst, yuyv);
    }


    void yuyv_to_rgba(SpanView<u8> const& src, img::SubView const& dst, PixelFormat format)
    {
        assert(src.length == dst.width * dst.height * 2);

        auto yuyv = offset_yuyv(format);
        span_yuyv_to_sub_view(src, dst, yuyv);
    }
    

    template <class VIEW>
    void nv12_to_rgba(SpanView<u8> const& src, VIEW const& dst)
    {
        auto const width = dst.width;
        auto const height = dst.height;

        assert(src.length == width * height + width * height / 2);

        auto sy = src.begin;
        auto suv = sy + width * height;
        auto sd = img::row_begin(dst, 0);

        auto u = suv;
        auto v = u + 1;        

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
}


/* to yuv planar */

namespace convert
{
    static void yuyv_to_planar_scale(SpanView<u8> const& src, u32 width, u32 height, ViewYUV const& dst, OffsetYUYV yuyv)
    {
        img::View1<u32> v32{};
        v32.width = width;
        v32.height = height;
        v32.matrix_data_ = (u32*)src.begin;

        auto scale = height / dst.height;

        switch (scale)
        {
        case 1:
            yuyv_to_planar(v32, dst, yuyv);
            break;

        case 2:
        case 4:
        case 8:
            yuyv_to_planar(v32, dst, yuyv, scale);
            break;
        
        default:
            break;
        }
    }


    static void nv12_to_planar_scale(SpanView<u8> const& src, u32 width, u32 height, ViewYUV const& dst)
    {
        img::View1u8 src_y{};
        src_y.width = width;
        src_y.height = height;
        src_y.matrix_data_ = src.begin;

        img::View1<UV> src_uv{};
        src_uv.width = width / 2;
        src_uv.height = height / 2;
        src_uv.matrix_data_ = (UV*)(src.begin + width * height);

        auto scale = height / dst.height;

        switch (scale)
        {
        case 1:
            nv12_to_planar(src_y, src_uv, dst);
            break;

        case 2:
        case 4:
        case 8:
            nv12_to_planar(src_y, src_uv, dst, scale);
            break;

        default:
            break;
        }
    }


    static void yuyv_to_yuv(SpanView<u8> const& src, u32 width, u32 height, ViewYUV const& dst, PixelFormat format)
    {
        assert(src.length == width * height * 2);

        auto yuyv = offset_yuyv(format);
        yuyv_to_planar_scale(src, width, height, dst, yuyv);
    }


    static void nv12_to_yuv(SpanView<u8> const& src, u32 width, u32 height, ViewYUV const& dst)
    { 
        //                  |--- nv12 y ---| |------- nv12 uv ------|
        assert(src.length == width * height + 2 * width * height / 4);

        nv12_to_planar_scale(src, width, height, dst);
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
            nv12_to_rgba(src, dst);
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
            nv12_to_rgba(src, dst);
            break;

        default:
            img::fill(dst, img::to_pixel(100));
        }
    }
    

    ViewYUV make_view_yuv(u32 width, u32 height, img::Buffer8& buffer)
    {
        auto len = width * height;

        ViewYUV view{};

        view.width = width;
        view.height = height;

        view.channel_data[0] = mb::push_elements(buffer, len);
        view.channel_data[1] = mb::push_elements(buffer, len);
        view.channel_data[2] = mb::push_elements(buffer, len);

        return view; 
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
            nv12_to_yuv(src, width, height, dst);
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
}


