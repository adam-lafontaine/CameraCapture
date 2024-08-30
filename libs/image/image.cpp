#pragma once

#include "image.hpp"
#include "../util/numeric.hpp"
#include "../stb_image/stb_image.h"
#include "../stb_image/stb_image_resize2.h"
#include "../stb_image/stb_image_write.h"

#include <cassert>


namespace sp = span;
namespace num = numeric;


namespace image
{
    bool create_image(Image& image, u32 width, u32 height)
    {
        auto data = mem::malloc<Pixel>(width * height, "create_image");
        if (!data)
        {
            return false;
        }

        image.data_ = data;
        image.width = width;
        image.height = height;

        return true;
    }

    
    void destroy_image(Image& image)
    {
        if (image.data_)
		{
			mem::free(image.data_);
			image.data_ = nullptr;
		}

		image.width = 0;
		image.height = 0;
    }
}


/* make_view */

namespace image
{
    ImageView make_view(Image const& image)
    {
        ImageView view{};

        view.width = image.width;
        view.height = image.height;
        view.matrix_data_ = image.data_;

        return view;
    }


    ImageView make_view(u32 width, u32 height, Buffer32& buffer)
    {
        ImageView view{};

        view.matrix_data_ = mb::push_elements(buffer, width * height);
        if (view.matrix_data_)
        {
            view.width = width;
            view.height = height;
        }

        return view;
    }


    GrayView make_view(u32 width, u32 height, Buffer8& buffer)
    {
        GrayView view{};

        view.matrix_data_ = mb::push_elements(buffer, width * height);
        if (view.matrix_data_)
        {
            view.width = width;
            view.height = height;
        }

        return view;
    }
}


/* fill */

namespace image
{
    template <typename T>
    static void fill_span_if(SpanView<T> const& dst, u8 value, fn<bool(T)> const& pred)
    {
        T* d = dst.begin;
        for (u32 i = 0; i < dst.length; ++i)
		{
			d[i] = pred(d[i]) ? value : d[i];
		}
    }
    

    void fill(ImageView const& view, Pixel color)
    {
        assert(view.matrix_data_);

        sp::fill_span_32(to_span(view), color);
    }


    void fill(SubView const& view, Pixel color)
    {
        assert(view.matrix_data_);

        for (u32 y = 0; y < view.height; y++)
        {
            sp::fill_span_32(row_span(view, y), color);
        }
    }


    void fill_if(GraySubView const& view, u8 gray, fn<bool(u8)> const& pred)
    {
        assert(view.matrix_data_);

        for (u32 y = 0; y < view.height; y++)
        {
            fill_span_if(row_span(view, y), gray, pred);
        }
    }
}


/* copy */

namespace image
{
    template <class VIEW_S, class VIEW_D>
    static void copy_view(VIEW_S const& src, VIEW_D const& dst)
    {
        sp::copy_span(to_span(src), to_span(dst));
    }


    void copy(ImageView const& src, ImageView const& dst)
    {
        assert(src.matrix_data_);
        assert(dst.matrix_data_);
        assert(dst.width == src.width);
        assert(dst.height == src.height);

        copy_view(src, dst);
    }


    void copy(GrayView const& src, GrayView const& dst)
    {
        assert(src.matrix_data_);
        assert(dst.matrix_data_);
        assert(dst.width == src.width);
        assert(dst.height == src.height);

        copy_view(src, dst);
    }
}


/* alpha_blend */

namespace image
{  
    static constexpr f32 alpha_scale(u8 alpha)
    {
        constexpr auto scale = 1.0f / 255.0f;

        switch (alpha)
        {
        case 0: return 0.0f;

        case 255: return 1.0f;

        case 126:
        case 127:
        case 128:
        case 129:
            return 0.5f;

        default: return alpha * scale;
        }
    }
    
    
    static inline void alpha_blend(u8 r, u8 g, u8 b, u8 a, Pixel* dst)
    {
        auto alpha = alpha_scale(a);
        auto i = 1.0f - alpha;

        auto& d = *dst;
        d.red = num::round_to_unsigned<u8>(num::fmaf(alpha, r, i * d.red));
        d.green = num::round_to_unsigned<u8>(num::fmaf(alpha, g, i * d.green));
        d.blue = num::round_to_unsigned<u8>(num::fmaf(alpha, b, i * d.blue));
    }
    
    
    static inline void alpha_blend(Pixel src, Pixel* dst, f32 alpha)
    {        
        auto const i = 1.0f - alpha;

        auto& d = *dst;
        d.red = num::round_to_unsigned<u8>(num::fmaf(alpha, src.red, i * d.red) /* a * s.red + i * d.red */);
        d.green = num::round_to_unsigned<u8>(num::fmaf(alpha, src.green, i * d.green) /* a * s.green + i * d.green */);
        d.blue = num::round_to_unsigned<u8>(num::fmaf(alpha, src.blue, i * d.blue) /* a * s.blue + i * d.blue */);
    }


    static inline void alpha_blend(Pixel* src, Pixel* dst, f32 alpha)
    {
        alpha_blend(*src, dst, alpha);
    }


    static void alpha_blend(Pixel s, Pixel* dst)
    {
        alpha_blend(s, dst, alpha_scale(s.alpha));
    }    


    static void alpha_blend(Pixel* src, Pixel* dst)
    {
        alpha_blend(*src, dst);
    }


    static void alpha_blend_span(SpanView<Pixel> const& src, SpanView<Pixel> const& dst)
    {
        for (u32 i = 0; i < dst.length; ++i) // TODO: simd
        {
            alpha_blend(src.begin + i, dst.begin + i);
        }
    }


    static void alpha_blend_span(SpanView<Pixel> const& src, SpanView<Pixel> const& dst, f32 alpha)
    {
        for (u32 i = 0; i < dst.length; ++i) // TODO: simd
        {
            alpha_blend(src.begin + i, dst.begin + i, alpha);
        }
    }
}


/* transform static */

namespace image
{
    template <typename P_SRC, typename P_DST>
    static void transform_span(SpanView<P_SRC> const& src, SpanView<P_DST> const& dst, fn<P_DST(P_SRC, P_DST)> const& func)
    {
        auto s = src.begin;
        auto d = dst.begin;

        for (u32 i = 0; i < src.length; i++)
        {
            d[i] = func(s[i], d[i]);
        }
    }
    

    template <typename P_SRC, typename P_DST>
    static void transform_span(SpanView<P_SRC> const& src, SpanView<P_DST> const& dst, fn<P_DST(P_SRC)> const& func)
    {
        auto s = src.begin;
        auto d = dst.begin;

        for (u32 i = 0; i < src.length; i++)
        {
            d[i] = func(s[i]);
        }
    }


    template <class V_SRC, class V_DST, class FUNC>
    static void transform_view(V_SRC const& src, V_DST const& dst, FUNC const& func)
    {
        transform_span(to_span(src), to_span(dst), func);
    }


    template <class V_SRC, class V_DST, class FUNC>
    static void transform_sub_view(V_SRC const& src, V_DST const& dst, FUNC const& func)
    {
        for (u32 y = 0; y < src.height; y++)
        {
            transform_span(row_span(src, y), row_span(dst, y), func);
        }
    }

}


/* transform api */

namespace image
{
    void transform(ImageView const& src, GrayView const& dst, fn<u8(Pixel)> const& func)
    {
        assert(src.matrix_data_);
        assert(dst.matrix_data_);
        assert(dst.width == src.width);
        assert(dst.height == src.height);

        transform_view(src, dst, func);
    }


    void transform(GrayView const& src, SubView const& dst, fn<Pixel(u8, Pixel)> const& func)
    {
        assert(src.matrix_data_);
        assert(dst.matrix_data_);
        assert(dst.width == src.width);
        assert(dst.height == src.height);

        transform_sub_view(src, dst, func);
    }


    void transform(GraySubView const& src, SubView const& dst, fn<Pixel(u8, Pixel)> const& func)
    {
        assert(src.matrix_data_);
        assert(dst.matrix_data_);
        assert(dst.width == src.width);
        assert(dst.height == src.height);

        transform_sub_view(src, dst, func);
    }
}


/* for_each_pixel */

namespace image
{
    template <typename T>
    static inline void for_each_in_span(SpanView<T> const& span, fn<void(T)> const& func)
    {
        for (u32 i = 0; i < span.length; i++)
        {
            func(span.begin[i]);
        }
    }


    void for_each_pixel(ImageView const& view, fn<void(Pixel)> const& func)
    {
        assert(view.matrix_data_);

        for_each_in_span(to_span(view), func);
    }


    void for_each_pixel(GrayView const& view, fn<void(u8)> const& func)
    {
        assert(view.matrix_data_);

        for_each_in_span(to_span(view), func);
    }
}


/* read write */

namespace image
{
    static bool has_extension(const char* filename, const char* ext)
    {
        auto file_length = span::strlen(filename);
        auto ext_length = span::strlen(ext);

        return !span::strcmp(&filename[file_length - ext_length], ext);
    }


    static bool is_bmp(const char* filename)
    {
        return has_extension(filename, ".bmp") || has_extension(filename, ".BMP");
    }


    static bool is_png(const char* filename)
    {
        return has_extension(filename, ".png") || has_extension(filename, ".PNG");
    }


    static bool is_valid_image_file(const char* filename)
    {
        return 
            has_extension(filename, ".bmp") || 
            has_extension(filename, ".BMP") ||
            has_extension(filename, ".png")||
            has_extension(filename, ".PNG");
    }


    bool read_image_from_file(const char* img_path_src, Image& image_dst)
	{
#ifdef IMAGE_READ
        auto is_valid_file = is_valid_image_file(img_path_src);
        assert(is_valid_file && "invalid image file");

        if (!is_valid_file)
        {
            return false;
        }

		int width = 0;
		int height = 0;
		int image_channels = 0;
		int desired_channels = 4;

		auto data = (Pixel*)stbi_load(img_path_src, &width, &height, &image_channels, desired_channels);

		assert(data && "stbi_load() - no image data");
		assert(width && "stbi_load() - no image width");
		assert(height && "stbi_load() - no image height");

		if (!data)
		{
			return false;
		}

		image_dst.data_ = data;
		image_dst.width = width;
		image_dst.height = height;

        mem::tag(data, image_dst.width * image_dst.height, "stbi_load");

		return true;
#else

        assert(false && " *** IMAGE_READ not enabled *** ");
        return false;

#endif
	}


    bool write_image(Image const& image_src, const char* file_path_dst)
	{
#ifdef IMAGE_WRITE
		assert(image_src.width);
		assert(image_src.height);
		assert(image_src.data_);

		int width = (int)(image_src.width);
		int height = (int)(image_src.height);
		int channels = 4;
		auto const data = image_src.data_;

		int result = 0;

		if(is_bmp(file_path_dst))
		{
			result = stbi_write_bmp(file_path_dst, width, height, channels, data);
			assert(result);
		}
		else if(is_png(file_path_dst))
		{
			int stride_in_bytes = width * channels;

			result = stbi_write_png(file_path_dst, width, height, channels, data, stride_in_bytes);
			assert(result);
		}
		else
		{
			assert(false && " *** bad image file type *** ");
            return false;
		}

		return (bool)result;
#else

        assert(false && " *** IMAGE_WRITE not enabled *** ");
        return false;

#endif
	}

}


/* make_view */

namespace image
{
    template <typename T, u32 C>
	static inline void make_view_n(ChannelMatrix2D<T, C>& view, u32 width, u32 height, MemoryBuffer<T>& buffer)
	{
		view.width = width;
		view.height = height;

		for (u32 ch = 0; ch < C; ++ch)
		{
			view.channel_data[ch] = mb::push_elements(buffer, width * height);
		}
	}


    View3u8 make_view_3(u32 width, u32 height, Buffer8& buffer)
    {
        View3u8 view{};

        make_view_n(view, width, height, buffer);

        return view;
    }


    View4u8 make_view_4(u32 width, u32 height, Buffer8& buffer)
    {
        View4u8 view{};

        make_view_n(view, width, height, buffer);

        return view;
    }
}


/* channel span */

namespace image
{
    template <typename T, u32 C>
    class ChannelSpan
    {
    public:
        u32 length = 0;

        T* channel_data[C] = { 0 };
    };


    template <typename T, u32 C>
    static ChannelSpan<T, C> to_span(ChannelMatrix2D<T, C> const& view)
    {
        ChannelSpan<T, C> span{};

        span.length = view.width * view.height;

        for (u32 i = 0; i < C; i++)
        {
            span.channel_data[i] = view.channel_data[i];
        }

        return span;
    }


    using SpanRGBAu8 = ChannelSpan<u8, 4>;
    using SpanRGBu8 = ChannelSpan<u8, 3>;


    template <typename T, u32 C>
	static inline ChannelSpan<T, C> row_span(ChannelMatrix2D<T, C> const& view, u32 y)
	{
        ChannelSpan<T, C> span{};

        span.length = view.width;

        auto offset = (u64)y * view.width;

        for (u32 i = 0; i < C; i++)
        {
            span.channel_data[i] = view.channel_data[i] + offset;
        }        

        return span;
	}


    template <typename T, u32 C>
    static inline ChannelSpan<T, C> row_span(ChannelSubView2D<T, C> const& view, u32 y)
    {
        ChannelSpan<T, C> span{};

        span.length = view.width;

        auto offset = (u64)(view.y_begin + y) * view.channel_width + view.x_begin;

        for (u32 i = 0; i < C; i++)
        {
            span.channel_data[i] = view.channel_data[i] + offset;
        }        

        return span;
    }
}


/* map_rgba */

namespace image
{
    static void map_span_rgba(SpanView<Pixel> const& src, SpanRGBAu8 const& dst)
    {
        auto r = dst.channel_data[(int)RGBA::R];
        auto g = dst.channel_data[(int)RGBA::G];
        auto b = dst.channel_data[(int)RGBA::B];
        auto a = dst.channel_data[(int)RGBA::A];

        for (u32 i = 0; i < src.length; i++)
        {
            auto p = src.begin[i];
            r[i] = p.red;
            g[i] = p.green;
            b[i] = p.blue;
            a[i] = p.alpha;
        }
    }


    static void map_span_rgba(SpanRGBu8 const& src, SpanView<Pixel> const& dst)
    {
        auto r = src.channel_data[(int)RGB::R];
        auto g = src.channel_data[(int)RGB::G];
        auto b = src.channel_data[(int)RGB::B];

        for (u32 i = 0; i < src.length; i++)
        {
            auto& p = dst.begin[i];
            p.red = r[i];
            p.green = g[i];
            p.blue = b[i];
            p.alpha = 255;
        }
    }


    static void map_span_blend(SpanRGBAu8 const& src, SpanView<Pixel> const& dst)
    {
        auto r = src.channel_data[(int)RGBA::R];
        auto g = src.channel_data[(int)RGBA::G];
        auto b = src.channel_data[(int)RGBA::B];
        auto a = src.channel_data[(int)RGBA::A];

        for (u32 i = 0; i < src.length; i++)
        {
            auto p = dst.begin + i;
            alpha_blend(r[i], g[i], b[i], a[i], p);
        }
    }


    void map_rgba(ImageView const& src, View4u8 const& dst)
    {
        assert(src.matrix_data_);
        assert(dst.channel_data[0]);
        assert(src.width == dst.width);
        assert(src.height == dst.height);

        map_span_rgba(to_span(src), to_span(dst));
    }


    void map_rgba(ImageView const& src, SubView4u8 const& dst)
    {
        assert(src.matrix_data_);
        assert(dst.channel_data[0]);
        assert(src.width == dst.width);
        assert(src.height == dst.height);

        for (u32 y = 0; y < src.height; y++)
        {
            map_span_rgba(row_span(src, y), row_span(dst, y));
        }
    }


    void map_rgba(View3u8 const& src, ImageView const& dst)
    {
        assert(dst.matrix_data_);
        assert(src.channel_data[0]);
        assert(src.width == dst.width);
        assert(src.height == dst.height);

        map_span_rgba(to_span(src), to_span(dst));
    }

    
}