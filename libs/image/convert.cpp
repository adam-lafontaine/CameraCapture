#pragma once

#include "convert.hpp"

#include <jpeglib.h>


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
    

    static void yuyv_to_planar(SpanView<u8> const& src, ViewYUV const& dst, OffsetYUYV yuyv)
    {
        auto const len  = src.length;

        auto s = src.begin;

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
    

    static void yuyv_to_planar2(img::View1<u32> const& src, ViewYUV const& dst, OffsetYUYV yuyv)
    {
        auto y1 = yuyv.y1;
        auto y2 = yuyv.y2;
        auto u = yuyv.u;
        auto v = yuyv.v;

        auto sw = src.width;
        auto sh = dst.width;

        auto dw = dst.width;
        auto dh = dst.height;

        u32 const N = sh / dh;

        constexpr u32 N_MAX = 8;

        assert(N == 1 || N % 2 == 0);
        assert(N <= N_MAX);

        u32 N2 = N / 2;
        u32 NN = N * N;
        f32 iNN = 1.0f / (NN);

        auto dy = img::select_channel(dst, (u32)YUV::Y);
        auto du = img::select_channel(dst, (u32)YUV::U);
        auto dv = img::select_channel(dst, (u32)YUV::V);

        auto sr = img::make_rect(N2, N);

        u32 dr = 0; // dst row
        u32 dc = 0; // dst column

        f32 yf = 0.0f;
        f32 uf = 0.0f;
        f32 vf = 0.0f;
        
        for (dr = 0; dr < dh; dr++)
        {
            auto drow_y = img::row_begin(dy, dr);
            auto drow_u = img::row_begin(du, dr);
            auto drow_v = img::row_begin(dv, dr);

            for (dc = 0; dc < dw; dc++)
            {
                yf = uf = vf = 0.0f;

                auto sview = img::sub_view(src, sr);
                for (u32 y = 0; y < sview.height; y++)
                {
                    auto srow = img::row_begin(sview, y);
                    for (u32 x = 0; x < sview.width; x++)
                    {
                        auto s8 = (u8*)(srow + x);
                        yf += s8[y1] + s8[y2];
                        uf += s8[u];
                        vf += s8[v];
                    }
                }

                drow_y[dc] = yf * iNN;
                drow_u[dc] = uf * 2 * iNN;
                drow_v[dc] = vf * 2 * iNN;

                sr.x_begin += N2;
                sr.x_end += N2;
            }

            sr.y_begin += N;
            sr.y_end += N;
        }
    }


    


    template <u32 N>
    void nv12_to_planar2(SpanView<u8> const& src, ViewYUV const& dst)
    {
        struct UV
        {
            u8 u;
            u8 v;
        };

        static_assert(N % 2 == 0);

        constexpr u32 N2 = N / 2;
        constexpr u32 NN = N * N;
        constexpr f32 iNN = 1.0f / (NN);

        auto dw = dst.width;
        auto dh = dst.height;

        auto sy = src.begin;
        auto suv = (UV*)(src.begin + dw * dh);

        auto dy = dst.channel_data[(int)YUV::Y];
        auto du = dst.channel_data[(int)YUV::U];
        auto dv = dst.channel_data[(int)YUV::V];

        u32 sw = dw; // src width;

        u32 dr = 0; // dst row
        u32 dc = 0; // dst column
        u32 sr = 0; // src row
        u32 sc = 0; // src column

        f32 yf = 0.0f;
        f32 uf = 0.0f;
        f32 vf = 0.0f;

        u32 i = 0;
        for (; dr < dh; dr++)
        {
            for (; dc < dw; dc++)
            {
                sw = dw;
                
                yf = 0.0f;
                sr = N * dr;
                for (u32 nr = 0; nr < N; nr++)
                {
                    sc = N * dc;
                    auto s = sy + sr * sw + sc;
                    for (u32 nc = 0; nc < N; nc++)
                    {                        
                        yf += s[nc];
                    }

                    sr++;
                }

                sw = dw / 2;
                
                uf = vf = 0.0f;
                sr = N2 * dr;
                for (u32 nr = 0; nr < N2; nr++)
                {
                    sc = N2 * dc;
                    auto s = suv + sr * sw + sc;
                    for (u32 nc = 0; nc < N2; nc++)
                    {
                        auto uv = s[nc];
                        uf += uv.u;
                        vf += uv.v;
                    }

                    sr++;
                }

                dy[i] = num::round_to_unsigned<u8>(yf * iNN);

                du[i] = num::round_to_unsigned<u8>(uf * 4 * iNN);
                dy[i] = num::round_to_unsigned<u8>(vf * 4 * iNN);

                i++;
            }
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


    static void yuv_to_rgb(u8* y, u8* u, u8* v, u8* r, u8* g, u8* b, u32 len)
    {
        // TODO: simd

        for (u32 i = 0; i < len; i++)
        {
            auto yf = y[i] * yuv::f;
            auto uf = u[i] * yuv::f - 0.5f;
            auto vf = v[i] * yuv::f - 0.5f;

            auto rf = num::clamp((yuv::yr * yf) + (yuv::ur * uf) + (yuv::vr * vf), 0.0f, 1.0f) * 255;
            auto gf = num::clamp((yuv::yg * yf) + (yuv::ug * uf) + (yuv::vg * vf), 0.0f, 1.0f) * 255;
            auto bf = num::clamp((yuv::yb * yf) + (yuv::ub * uf) + (yuv::vb * vf), 0.0f, 1.0f) * 255;

            r[i] = num::round_to_unsigned<u8>(rf);
            g[i] = num::round_to_unsigned<u8>(gf);
            b[i] = num::round_to_unsigned<u8>(bf);
        }
    }


    static void yuv_to_rgb(u8* y, u8* u, u8* v, img::Pixel* p, u32 len)
    {
        for (u32 i = 0; i < len; i++)
        {
            auto yf = y[i] * yuv::f;
            auto uf = u[i] * yuv::f - 0.5f;
            auto vf = v[i] * yuv::f - 0.5f;

            auto rf = num::clamp((yuv::yr * yf) + (yuv::ur * uf) + (yuv::vr * vf), 0.0f, 1.0f) * 255;
            auto gf = num::clamp((yuv::yg * yf) + (yuv::ug * uf) + (yuv::vg * vf), 0.0f, 1.0f) * 255;
            auto bf = num::clamp((yuv::yb * yf) + (yuv::ub * uf) + (yuv::vb * vf), 0.0f, 1.0f) * 255;

            p[i].red   = num::round_to_unsigned<u8>(rf);
            p[i].green = num::round_to_unsigned<u8>(gf);
            p[i].blue  = num::round_to_unsigned<u8>(bf);
        }
    }    


    static void yuv_to_rgba(u8* y, u8* u, u8* v, img::Pixel* dst, u32 len)
    {
        for (u32 i = 0; i < len; i++)
        {
            auto yf = y[i] * yuv::f;
            auto uf = u[i] * yuv::f - 0.5f;
            auto vf = v[i] * yuv::f - 0.5f;

            auto rf = num::clamp((yuv::yr * yf) + (yuv::ur * uf) + (yuv::vr * vf), 0.0f, 1.0f) * 255;
            auto gf = num::clamp((yuv::yg * yf) + (yuv::ug * uf) + (yuv::vg * vf), 0.0f, 1.0f) * 255;
            auto bf = num::clamp((yuv::yb * yf) + (yuv::ub * uf) + (yuv::vb * vf), 0.0f, 1.0f) * 255;

            dst[i].red   = num::round_to_unsigned<u8>(rf);
            dst[i].green = num::round_to_unsigned<u8>(gf);
            dst[i].blue  = num::round_to_unsigned<u8>(bf);
            dst[i].alpha = 255;
        }
    }
    
    
    static void yuv_to_rgba(u8 y, u8 u, u8 v, img::Pixel* dst)
    {
        auto yf = y * yuv::f;
        auto uf = u * yuv::f - 0.5f;
        auto vf = v * yuv::f - 0.5f;

        auto rf = num::clamp((yuv::yr * yf) + (yuv::ur * uf) + (yuv::vr * vf), 0.0f, 1.0f) * 255;
        auto gf = num::clamp((yuv::yg * yf) + (yuv::ug * uf) + (yuv::vg * vf), 0.0f, 1.0f) * 255;
        auto bf = num::clamp((yuv::yb * yf) + (yuv::ub * uf) + (yuv::vb * vf), 0.0f, 1.0f) * 255;

        dst->red   = num::round_to_unsigned<u8>(rf);
        dst->green = num::round_to_unsigned<u8>(gf);
        dst->blue  = num::round_to_unsigned<u8>(bf);
        dst->alpha = 255;
    }


    static void yuv_to_rgb(f32* y, f32* u, f32* v, u8* r, u8* g, u8* b, u32 len)
    {
        for (u32 i = 0; i < len; i++)
        {
            auto yf = y[i];
            auto uf = u[i] - 127.5f;
            auto vf = v[i] - 127.5f;

            auto rf = (yuv::yr * yf) + (yuv::ur * uf) + (yuv::vr * vf);
            auto gf = (yuv::yg * yf) + (yuv::ug * uf) + (yuv::vg * vf);
            auto bf = (yuv::yb * yf) + (yuv::ub * uf) + (yuv::vb * vf);

            r[i] = num::round_to_unsigned<u8>(num::clamp(rf, 0.0f, 255.0f));
            g[i] = num::round_to_unsigned<u8>(num::clamp(gf, 0.0f, 255.0f));
            b[i] = num::round_to_unsigned<u8>(num::clamp(bf, 0.0f, 255.0f));
        }
    }


    static void yuv_to_rgb(f32* y, f32* u, f32* v, img::Pixel* dst, u32 len)
    {
        for (u32 i = 0; i < len; i++)
        {
            auto yf = y[i];
            auto uf = u[i] - 127.5f;
            auto vf = v[i] - 127.5f;

            auto rf = (yuv::yr * yf) + (yuv::ur * uf) + (yuv::vr * vf);
            auto gf = (yuv::yg * yf) + (yuv::ug * uf) + (yuv::vg * vf);
            auto bf = (yuv::yb * yf) + (yuv::ub * uf) + (yuv::vb * vf);

            auto& p = dst[i];

            p.red   = num::round_to_unsigned<u8>(num::clamp(rf, 0.0f, 255.0f));
            p.green = num::round_to_unsigned<u8>(num::clamp(gf, 0.0f, 255.0f));
            p.blue  = num::round_to_unsigned<u8>(num::clamp(bf, 0.0f, 255.0f));
        }
    }


    static void yuv_to_rgba(f32* y, f32* u, f32* v, img::Pixel* dst, u32 len)
    {
        for (u32 i = 0; i < len; i++)
        {
            auto yf = y[i];
            auto uf = u[i] - 127.5f;
            auto vf = v[i] - 127.5f;

            auto rf = (yuv::yr * yf) + (yuv::ur * uf) + (yuv::vr * vf);
            auto gf = (yuv::yg * yf) + (yuv::ug * uf) + (yuv::vg * vf);
            auto bf = (yuv::yb * yf) + (yuv::ub * uf) + (yuv::vb * vf);

            auto& p = dst[i];

            p.red   = num::round_to_unsigned<u8>(num::clamp(rf, 0.0f, 255.0f));
            p.green = num::round_to_unsigned<u8>(num::clamp(gf, 0.0f, 255.0f));
            p.blue  = num::round_to_unsigned<u8>(num::clamp(bf, 0.0f, 255.0f));
            p.alpha = 255;
        }
    }
}


namespace convert
{
    static void span_yuv4_to_pixel(SpanView<u8> const& src, SpanView<img::Pixel> const& dst, OffsetYUYV yuyv)
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
            yuv_to_rgba(s[y1], s[u], s[v], d1);
            yuv_to_rgba(s[y2], s[u], s[v], d2);

            d1 += 2;
            d2 += 2;

            s += 4;
        }
    }


    
}


/* to rgba */

namespace convert
{
    void mjpeg_to_rgba(SpanView<u8> const& src, img::ImageView const& dst)
    {
        auto format = mjpeg::image_format::RGBA8;

        mjpeg::convert(src.begin, dst.width, src.length, (u8*)dst.matrix_data_, format);
    }


    void yuyv_to_rgba(SpanView<u8> const& src, img::ImageView const& dst)
    {
        assert(src.length == dst.width * dst.height * 2);

        OffsetYUYV yuyv{};
        yuyv.y1 = 0;
        yuyv.u  = 1;
        yuyv.y2 = 2;
        yuyv.v  = 3;

        span_yuv4_to_pixel(src, img::to_span(dst), yuyv);
    }


    void yuyv_to_rgba(SpanView<u8> const& src, img::SubView const& dst)
    {
        assert(src.length == dst.width * dst.height * 2);

        OffsetYUYV yuyv{};
        yuyv.y1 = 0;
        yuyv.u  = 1;
        yuyv.y2 = 2;
        yuyv.v  = 3;

        auto const w = dst.width;

        for (u32 y = 0; y < dst.height; y++)
        {
            span_yuv4_to_pixel(span::sub_view(src, y * w, w), img::row_span(dst, y), yuyv);
        }
    }


    void yvyu_to_rgba(SpanView<u8> const& src, img::ImageView const& dst)
    {
        assert(src.length == dst.width * dst.height * 2);

        OffsetYUYV yuyv{};
        yuyv.y1 = 0;
        yuyv.v  = 1;
        yuyv.y2 = 2;
        yuyv.u  = 3;

        span_yuv4_to_pixel(src, img::to_span(dst), yuyv);
    }


    void yvyu_to_rgba(SpanView<u8> const& src, img::SubView const& dst)
    {
        assert(src.length == dst.width * dst.height * 2);

        OffsetYUYV yuyv{};
        yuyv.y1 = 0;
        yuyv.v  = 1;
        yuyv.y2 = 2;
        yuyv.u  = 3;

        auto const w = dst.width;

        for (u32 y = 0; y < dst.height; y++)
        {
            span_yuv4_to_pixel(span::sub_view(src, y * w, w), img::row_span(dst, y), yuyv);
        }
    }


    void uyvy_to_rgba(SpanView<u8> const& src, img::ImageView const& dst)
    {
        assert(src.length == dst.width * dst.height * 2);

        OffsetYUYV yuyv{};
        yuyv.u  = 0;
        yuyv.y1 = 1;
        yuyv.v  = 2;
        yuyv.y2 = 3;

        span_yuv4_to_pixel(src, img::to_span(dst), yuyv);
    }


    void uyvy_to_rgba(SpanView<u8> const& src, img::SubView const& dst)
    {
        assert(src.length == dst.width * dst.height * 2);

        OffsetYUYV yuyv{};
        yuyv.u  = 0;
        yuyv.y1 = 1;
        yuyv.v  = 2;
        yuyv.y2 = 3;

        auto const w = dst.width;

        for (u32 y = 0; y < dst.height; y++)
        {
            span_yuv4_to_pixel(span::sub_view(src, y * w, w), img::row_span(dst, y), yuyv);
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


/* to yuv planar */

namespace convert
{
    static void yuyv_to_yuv(SpanView<u8> const& src, ViewYUV const& dst)
    {
        assert(src.length == dst.width * dst.height * 2);

        OffsetYUYV yuyv{};
        yuyv.y1 = 0;
        yuyv.u  = 1;
        yuyv.y2 = 2;
        yuyv.v  = 3;

        yuyv_to_planar(src, dst, yuyv);
    }


    static void yvyu_to_yuv(SpanView<u8> const& src, ViewYUV const& dst)
    {
        assert(src.length == dst.width * dst.height * 2);

        OffsetYUYV yuyv{};
        yuyv.y1 = 0;
        yuyv.v  = 1;
        yuyv.y2 = 2;
        yuyv.u  = 3;

        yuyv_to_planar(src, dst, yuyv);
    }


    static void uyvy_to_yuv(SpanView<u8> const& src, ViewYUV const& dst)
    {
        assert(src.length == dst.width * dst.height * 2);

        OffsetYUYV yuyv{};
        yuyv.u  = 0;
        yuyv.y1 = 1;
        yuyv.v  = 2;
        yuyv.y2 = 3;

        yuyv_to_planar(src, dst, yuyv);
    }


    static void nv12_to_yuv(SpanView<u8> const& src, ViewYUV const& dst)
    {
        struct UV
        {
            u8 u;
            u8 v;
        };

        auto width = dst.width;
        auto height = dst.height;
        auto len = width * height;

        auto sy = src.begin;
        auto suv = (UV*)(sy + len);

        auto dy = dst.channel_data[(int)YUV::Y];
        auto du = dst.channel_data[(int)YUV::U];
        auto dv = dst.channel_data[(int)YUV::V];

        for (u32 i = 0; i < len; i++)
        {
            dy[i] = (f32)sy[i];
        }

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

        case PF::MJPG:
            is_valid = 1;
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
            yuyv_to_rgba(src, dst);
            break;
        
        case PF::YVYU:
            yvyu_to_rgba(src, dst);
            break;
        
        case PF::UYVY:
        case PF::Y422:
        case PF::UYNV:
        case PF::HDYC:
            uyvy_to_rgba(src, dst);
            break;

        case PF::NV12:
            nv12_to_rgba(src, dst);
            break;
        
        case PF::MJPG:
            mjpeg_to_rgba(src, dst);
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
            yuyv_to_rgba(src, dst);
            break;
        
        case PF::YVYU:
            yvyu_to_rgba(src, dst);
            break;
        
        case PF::UYVY:
        case PF::Y422:
        case PF::UYNV:
        case PF::HDYC:
            uyvy_to_rgba(src, dst);
            break;

        case PF::NV12:
            nv12_to_rgba(src, dst);
            break;

        default:
            img::fill(dst, img::to_pixel(100));
        }
    }
    

    ViewYUV make_view_yuv(u32 width, u32 height, img::Buffer32& buffer)
    {
        auto len = width * height;

        ViewYUV view{};

        view.width = width;
        view.height = height;

        view.channel_data[0] = (f32*)mb::push_elements(buffer, len);
        view.channel_data[1] = (f32*)mb::push_elements(buffer, len);
        view.channel_data[2] = (f32*)mb::push_elements(buffer, len);

        return view; 
    }


    void to_yuv(SpanView<u8> const& src, ViewYUV const& dst, PixelFormat format)
    {
        using PF = PixelFormat;

        switch (format)
        {
        case PF::YUYV:
        case PF::YUNV:
        case PF::YUY2:
            yuyv_to_yuv(src, dst);
            break;
        
        case PF::YVYU:
            yvyu_to_yuv(src, dst);
            break;
        
        case PF::UYVY:
        case PF::Y422:
        case PF::UYNV:
        case PF::HDYC:
            uyvy_to_yuv(src, dst);
            break;

        case PF::NV12:
            nv12_to_yuv(src, dst);
            break;

        default:
            //img::fill(dst, img::to_pixel(100));
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


