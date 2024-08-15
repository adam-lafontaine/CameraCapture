#pragma once

#include "convert.hpp"

#include <jpeglib.h>


/* pixel format */

namespace convert
{  
   


    
}


namespace mjpeg
{
    enum class image_format : int
    {
        UNKNOWN,
        RGB8,
        RGBA8,
        GRAY8
    };


    struct error_mgr
    {
        struct jpeg_error_mgr super;
        jmp_buf jmp;
    };

    static void _error_exit(j_common_ptr dinfo)
    {
        struct error_mgr* myerr = (struct error_mgr*)dinfo->err;
        (*dinfo->err->output_message)(dinfo);
        longjmp(myerr->jmp, 1);
    }


    /* ISO/IEC 10918-1:1993(E) K.3.3. Default Huffman tables used by MJPEG UVC devices
        which don't specify a Huffman table in the JPEG stream. */
    static const unsigned char dc_lumi_len[] =
        {0, 0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0};
    static const unsigned char dc_lumi_val[] =
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

    static const unsigned char dc_chromi_len[] =
        {0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0};
    static const unsigned char dc_chromi_val[] =
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

    static const unsigned char ac_lumi_len[] =
        {0, 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d};
    static const unsigned char ac_lumi_val[] =
        {0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21,
        0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07, 0x22, 0x71,
        0x14, 0x32, 0x81, 0x91, 0xa1, 0x08, 0x23, 0x42, 0xb1,
        0xc1, 0x15, 0x52, 0xd1, 0xf0, 0x24, 0x33, 0x62, 0x72,
        0x82, 0x09, 0x0a, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x25,
        0x26, 0x27, 0x28, 0x29, 0x2a, 0x34, 0x35, 0x36, 0x37,
        0x38, 0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
        0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
        0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a,
        0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x83,
        0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x92, 0x93,
        0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3,
        0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3,
        0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
        0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3,
        0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
        0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf1,
        0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa};
    static const unsigned char ac_chromi_len[] =
        {0, 0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77};
    static const unsigned char ac_chromi_val[] =
        {0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31,
        0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71, 0x13, 0x22,
        0x32, 0x81, 0x08, 0x14, 0x42, 0x91, 0xa1, 0xb1, 0xc1,
        0x09, 0x23, 0x33, 0x52, 0xf0, 0x15, 0x62, 0x72, 0xd1,
        0x0a, 0x16, 0x24, 0x34, 0xe1, 0x25, 0xf1, 0x17, 0x18,
        0x19, 0x1a, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x35, 0x36,
        0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47,
        0x48, 0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
        0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
        0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a,
        0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a,
        0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a,
        0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa,
        0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba,
        0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca,
        0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
        0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
        0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa};

    #define COPY_HUFF_TABLE(dinfo, tbl, name)                        \
    do                                                               \
    {                                                                \
        if (dinfo->tbl == NULL)                                      \
            dinfo->tbl = jpeg_alloc_huff_table((j_common_ptr)dinfo); \
        memcpy(dinfo->tbl->bits, name##_len, sizeof(name##_len));    \
        memset(dinfo->tbl->huffval, 0, sizeof(dinfo->tbl->huffval)); \
        memcpy(dinfo->tbl->huffval, name##_val, sizeof(name##_val)); \
    } while (0)

    static void insert_huff_tables(j_decompress_ptr dinfo)
    {
        COPY_HUFF_TABLE(dinfo, dc_huff_tbl_ptrs[0], dc_lumi);
        COPY_HUFF_TABLE(dinfo, dc_huff_tbl_ptrs[1], dc_chromi);
        COPY_HUFF_TABLE(dinfo, ac_huff_tbl_ptrs[0], ac_lumi);
        COPY_HUFF_TABLE(dinfo, ac_huff_tbl_ptrs[1], ac_chromi);
    }
}


namespace mjpeg
{
    class jpeg_info_t
    {
    public:
        struct jpeg_decompress_struct dinfo;
        struct error_mgr jerr;

        u32 out_step = 0;
    };


    static bool setup_jpeg(jpeg_info_t& jinfo, u8* in_data, u32 in_width, u32 in_size, image_format out_format)
    {
        auto& jerr = jinfo.jerr;
        auto& dinfo = jinfo.dinfo;

        auto const fail = [&]()
        {
            jpeg_destroy_decompress(&dinfo);
            return false;
        };

        jerr.super.error_exit = _error_exit;
        if (setjmp(jerr.jmp))
        {
            return fail();
        }

        dinfo.err = jpeg_std_error(&jerr.super);

        jpeg_create_decompress(&dinfo);

        jpeg_mem_src(&dinfo, (unsigned char *)in_data, in_size);
        jpeg_read_header(&dinfo, TRUE);

        if (dinfo.dc_huff_tbl_ptrs[0] == NULL)
        {
            /* This frame is missing the Huffman tables: fill in the standard ones */
            insert_huff_tables(&dinfo);
        }

        switch (out_format)
        {
        case image_format::RGB8:
            dinfo.out_color_space = JCS_RGB;
            jinfo.out_step = in_width * 3;
            break;
        case image_format::RGBA8:
            dinfo.out_color_space = JCS_EXT_RGBA;
            jinfo.out_step = in_width * 4;
            break;
        case image_format::GRAY8:
            dinfo.out_color_space = JCS_GRAYSCALE;
            jinfo.out_step = in_width;
            break;
        default:
            return fail();
        }

        dinfo.dct_method = JDCT_IFAST;
        
        return true;
    }


    void convert(u8* in_data, u32 in_width, u32 in_size, u8* out_data, image_format out_format)
    {
        jpeg_info_t jinfo;
        if (!setup_jpeg(jinfo, in_data, in_width, in_size, out_format))
        {
            return;
        }

        auto& dinfo = jinfo.dinfo;

        constexpr u32 n_out_rows = 16;

        u8* out_array[n_out_rows] = { 0 };

        jpeg_start_decompress(&dinfo);

        size_t lines_read = 0;

        while (dinfo.output_scanline < dinfo.output_height)
        {
            for (u32 i = 0; i < n_out_rows; ++i)
            {
                out_array[i] = out_data + (lines_read + i) * jinfo.out_step;
            }           
            
            lines_read += jpeg_read_scanlines(&dinfo, out_array, 1);
        }

        jpeg_finish_decompress(&dinfo);
        jpeg_destroy_decompress(&dinfo);
    }
}


namespace convert
{
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

        dst->red   = num::round_to_unsigned<u8>(r);
        dst->green = num::round_to_unsigned<u8>(g);
        dst->blue  = num::round_to_unsigned<u8>(b);
        dst->alpha = 255;
    }


    static void span_yuyv_to_rgba(SpanView<u8> const& src, SpanView<img::Pixel> const& dst)
    {
        auto const len  = src.length;

        auto yuyv = src.begin;
        auto d = dst.begin;

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

            d1 += 2;
            d2 += 2;
        }
    }


    static void span_uyvy_to_rgba(SpanView<u8> const& src, SpanView<img::Pixel> const& dst)
    {
        auto const len  = src.length;

        auto uyvy = src.begin;
        auto d = dst.begin;

        auto y1 = uyvy;
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
            u += 4;
            y2 += 4;
            v += 4;

            d1 += 2;
            d2 += 2;
        }
    }


    static void span_yvyu_to_rgba(SpanView<u8> const& src, SpanView<img::Pixel> const& dst)
    {
        auto const len  = src.length;

        auto yuyv = src.begin;
        auto d = dst.begin;

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

            d1 += 2;
            d2 += 2;
        }
    }
}


namespace convert
{
    void mjpeg_to_rgba(SpanView<u8> const& src, img::ImageView const& dst)
    {
        auto format = mjpeg::image_format::RGBA8;

        mjpeg::convert(src.begin, dst.width, src.length, (u8*)dst.matrix_data_, format);
    }


    void yuyv_to_rgba(SpanView<u8> const& src , img::ImageView const& dst)
    {
        //assert(src.length == dst.width * dst.height);

        span_yuyv_to_rgba(src, img::to_span(dst));
    }


    void yuyv_to_rgba(SpanView<u8> const& src, img::SubView const& dst)
    {
        assert(src.length == dst.width * dst.height);

        auto const w = dst.width;

        for (u32 y = 0; y < dst.height; y++)
        {
            span_yuyv_to_rgba(span::sub_view(src, y * w, w), img::row_span(dst, y));
        }
    }


    void yvyu_to_rgba(SpanView<u8> const& src, img::ImageView const& dst)
    {
        assert(src.length == dst.width * dst.height);

        span_yvyu_to_rgba(src, img::to_span(dst));
    }


    void yvyu_to_rgba(SpanView<u8> const& src, img::SubView const& dst)
    {
        assert(src.length == dst.width * dst.height);

        auto const w = dst.width;

        for (u32 y = 0; y < dst.height; y++)
        {
            span_yvyu_to_rgba(span::sub_view(src, y * w, w), img::row_span(dst, y));
        }
    }


    void uyvy_to_rgba(SpanView<u8> const& src, img::ImageView const& dst)
    {
        assert(src.length == dst.width * dst.height);

        span_uyvy_to_rgba(src, img::to_span(dst));
    }


    void uyvy_to_rgba(SpanView<u8> const& src, img::SubView const& dst)
    {
        assert(src.length == dst.width * dst.height);

        auto const w = dst.width;

        for (u32 y = 0; y < dst.height; y++)
        {
            span_uyvy_to_rgba(span::sub_view(src, y * w, w), img::row_span(dst, y));
        }
    }
    
    
    void nv12_to_rgba(SpanView<u8> const& src, img::ImageView const& dst)
    {
        auto const width = dst.width;
        auto const height = dst.height;

        assert(src.length == width * height + width * height / 2);

        auto sy = src.begin;
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


    void nv12_to_rgba(SpanView<u8> const& src, img::SubView const& dst)
    {
        auto const width = dst.width;
        auto const height = dst.height;

        assert(src.length == width * height + width * height / 2);

        auto sy = src.begin;
        auto suv = sy + width * height;
        auto sd = img::row_span(dst, 0).begin;

        auto u = suv;
        auto v = u + 1;        

        for (u32 h = 0; h < height; h += 2)
        {
            auto y1 = sy + h * width;
            auto y2 = y1 + 1;
            auto y3 = y1 + width;
            auto y4 = y3 + 1;

            auto d1 = img::row_span(dst, h).begin;
            auto d2 = d1 + 1;
            auto d3 = img::row_span(dst, h + 1).begin;
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