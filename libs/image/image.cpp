#pragma once

#include "image.hpp"
#include "../util/numeric.hpp"
#include "../stb_image/stb_image.h"
#include "../stb_image/stb_image_resize2.h"
#include "../stb_image/stb_image_write.h"


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


/* row_begin */

namespace image
{
    template <typename T>
    static inline T* row_begin(MatrixView2D<T> const& view, u32 y)
    {
        return view.matrix_data_ + (u64)y * view.width;
    }


    template <typename T>
    static inline T* row_begin(MatrixSubView2D<T> const& view, u32 y)
    {
        return view.matrix_data_ + (u64)(view.y_begin + y) * view.matrix_width + view.x_begin;
    }
}


/* xy_at */

namespace image
{
    template <typename T>
    static inline T* xy_at(MatrixView2D<T> const& view, u32 x, u32 y)
    {
        return row_begin(view, y) + x;
    }


    template <typename T>
    static inline T* xy_at(MatrixSubView2D<T> const& view, u32 x, u32 y)
    {
        return row_begin(view, y) + x;
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


/* row_span */

namespace image
{
    template <typename T>
	static inline SpanView<T> row_span(MatrixView2D<T> const& view, u32 y)
	{
        SpanView<T> span{};

        span.begin = view.matrix_data_ + (u64)y * view.width;
        span.length = view.width;

        return span;
	}


    template <typename T>
    static inline SpanView<T> row_span(MatrixSubView2D<T> const& view, u32 y)
    {
        SpanView<T> span{};

        span.begin = view.matrix_data_ + (u64)(view.y_begin + y) * view.matrix_width + view.x_begin;
        span.length = view.width;

        return span;
    }


    template <typename T>
    static inline SpanView<T> to_span(MatrixView2D<T> const& view)
    {
        SpanView<T> span{};

        span.begin = view.matrix_data_;
        span.length = view.width * view.height;

        return span;
    }


    template <typename T>
    static inline SpanView<T> sub_span(MatrixView2D<T> const& view, u32 y, u32 x_begin, u32 x_end)
    {
        SpanView<T> span{};

        span.begin = view.matrix_data_ + (u64)(y * view.width) + x_begin;
        span.length = x_end - x_begin;

        return span;
    }


    template <typename T>
    static inline SpanView<T> sub_span(MatrixSubView2D<T> const& view, u32 y, u32 x_begin, u32 x_end)
    {
        SpanView<T> span{};

        span.begin = view.matrix_data_ + (u64)((view.y_begin + y) * view.matrix_width + view.x_begin) + x_begin;
        span.length = x_end - x_begin;

        return span;
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


/* read write */

namespace image
{
    static bool has_extension(const char* filename, const char* ext)
    {
        size_t file_length = std::strlen(filename);
        size_t ext_length = std::strlen(ext);

        return !std::strcmp(&filename[file_length - ext_length], ext);
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