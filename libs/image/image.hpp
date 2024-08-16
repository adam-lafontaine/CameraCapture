#pragma once

#include "../span/span.hpp"

#include <functional>

template <class F>
using fn = std::function<F>;


/*  image basic */

namespace image
{
    class RGBAu8
    {
    public:
        u8 red;
        u8 green;
        u8 blue;
        u8 alpha;
    };

    using Pixel = RGBAu8;
    using Image = Matrix2D<Pixel>;
    using ImageView = MatrixView2D<Pixel>;
    using ImageGray = Matrix2D<u8>;
    using GrayView = MatrixView2D<u8>;


    bool create_image(Image& image, u32 width, u32 height);

    void destroy_image(Image& image);


    inline u32 as_u32(Pixel p)
    {
        return  *((u32*)(&p));
    }


    inline Image as_image(ImageView const& view)
    {
        Image image;
        image.width = view.width;
        image.height = view.height;
        image.data_ = view.matrix_data_;

        return image;
    }
}


namespace image
{
    template <typename T>
    class MatrixSubView2D
    {
    public:
        T*  matrix_data_;
        u32 matrix_width;

        u32 x_begin;
        u32 y_begin;

        u32 width;
        u32 height;
    };


    using SubView = MatrixSubView2D<Pixel>;    
    using GraySubView = MatrixSubView2D<u8>;
}


namespace image
{
    using Buffer8 = MemoryBuffer<u8>;
    using Buffer32 = MemoryBuffer<Pixel>;


    constexpr inline Pixel to_pixel(u8 red, u8 green, u8 blue, u8 alpha)
    {
        Pixel p{};
        p.red = red;
        p.green = green;
        p.blue = blue;
        p.alpha = alpha;

        return p;
    }


    constexpr inline Pixel to_pixel(u8 red, u8 green, u8 blue)
    {
        return to_pixel(red, green, blue, 255);
    }


    constexpr inline Pixel to_pixel(u8 gray)
    {
        return to_pixel(gray, gray, gray);
    } 


    inline Buffer8 create_buffer8(u32 n_pixels)
	{
		Buffer8 buffer;
		mb::create_buffer(buffer, n_pixels);
		return buffer;
	}


    inline Buffer32 create_buffer32(u32 n_pixels, cstr tag)
	{
		Buffer32 buffer;
		mb::create_buffer(buffer, n_pixels, tag);
		return buffer;
	}


    inline Rect2Du32 make_rect(u32 width, u32 height)
    {
        Rect2Du32 range{};
        range.x_begin = 0;
        range.x_end = width;
        range.y_begin = 0;
        range.y_end = height;

        return range;
    }


    inline Rect2Du32 make_rect(u32 x_begin, u32 y_begin, u32 width, u32 height)
    {
        Rect2Du32 range{};
        range.x_begin = x_begin;
        range.x_end = x_begin + width;
        range.y_begin = y_begin;
        range.y_end = y_begin + height;

        return range;
    }
}


/* make_view */

namespace image
{
    ImageView make_view(Image const& image);

    ImageView make_view(u32 width, u32 height, Buffer32& buffer);

    GrayView make_view(u32 width, u32 height, Buffer8& buffer);
}


/* sub_view */

namespace image
{
    template <typename T>
    inline MatrixSubView2D<T> sub_view(MatrixView2D<T> const& view, Rect2Du32 const& range)
    {
        MatrixSubView2D<T> sub_view{};

        sub_view.matrix_data_ = view.matrix_data_;
        sub_view.matrix_width = view.width;
        sub_view.x_begin = range.x_begin;
        sub_view.y_begin = range.y_begin;
        sub_view.width = range.x_end - range.x_begin;
        sub_view.height = range.y_end - range.y_begin;

        return sub_view;
    }


    template <typename T>
    inline MatrixSubView2D<T> sub_view(MatrixSubView2D<T> const& view, Rect2Du32 const& range)
    {
        MatrixSubView2D<T> sub_view{};

        sub_view.matrix_data_ = view.matrix_data_;
        sub_view.matrix_width = view.matrix_width;

        sub_view.x_begin = range.x_begin + view.x_begin;
		sub_view.y_begin = range.y_begin + view.y_begin;

		sub_view.width = range.x_end - range.x_begin;
		sub_view.height = range.y_end - range.y_begin;

        return sub_view;
    }


    template <typename T>
    inline MatrixSubView2D<T> sub_view(MatrixView2D<T> const& view)
    {
        auto range = make_range(view.width, view.height);
        return sub_view(view, range);
    }


    inline Point2Du32 to_point(u32 x, u32 y)
    {
        Point2Du32 pt{};
        pt.x = x;
        pt.y = y;

        return pt;
    }
}


/* fill */

namespace image
{
    void fill(ImageView const& view, Pixel color);

    void fill(SubView const& view, Pixel color);

    void fill_if(GraySubView const& view, u8 gray, fn<bool(u8)> const& pred);
}


/* copy */

namespace image
{
    void copy(ImageView const& src, ImageView const& dst);
}


/* transform */

namespace image
{
    void transform(ImageView const& src, GrayView const& dst, fn<u8(Pixel)> const& func);

    void transform(GrayView const& src, SubView const& dst, fn<Pixel(u8, Pixel)> const& func);

    void transform(GraySubView const& src, SubView const& dst, fn<Pixel(u8, Pixel)> const& func);
}


/* for_each_pixel */

namespace image
{
    void for_each_pixel(ImageView const& view, fn<void(Pixel)> const& func);
}


/* read write */

namespace image
{
    bool read_image_from_file(const char* img_path_src, Image& image_dst);

    bool write_image(Image const& image_src, const char* file_path_dst);
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


/* planar channels */

namespace image
{
    template <typename T, u32 C>
	class ChannelMatrix2D
	{
	public:
		T* channel_data[C] = { 0 };

		u32 width = 0;
		u32 height = 0;
	};


	template <typename T>
	using View4 = ChannelMatrix2D<T, 4>;

	template <typename T>
	using View3 = ChannelMatrix2D<T, 3>;

	template <typename T>
	using View2 = ChannelMatrix2D<T, 2>;

	template <typename T>
	using View1 = MatrixView2D<T>;

    using View4u8 = View4<u8>;
    using View3u8 = View3<u8>;
    using View2u8 = View2<u8>;

	using View1u8 = GrayView;

    using ViewRGBAu8 = View4u8;
    using ViewRGBu8 = View3u8;


    enum class RGB : int { R = 0, G = 1, B = 2 };

	enum class RGBA : int {	R = 0, G = 1, B = 2, A = 3 };


    template <typename T, u32 C>
    class ChannelSubView2D
    {
    public:
        T* channel_data[C] = { 0 };
        u32 channel_width;

        u32 x_begin;
        u32 y_begin;

        u32 width;
        u32 height;
    };


    template <typename T>
    using SubView4 = ChannelSubView2D<T, 4>;

    template <typename T>
    using SubView3 = ChannelSubView2D<T, 3>;

    using SubView4u8 = SubView4<u8>;
    using SubView3u8 = SubView3<u8>;

    using SubViewRGBAu8 = SubView4u8;
    using SubViewRGBu8 = SubView3u8;
}


/* make_view */

namespace image
{
    View3u8 make_view_3(u32 width, u32 height, Buffer8& buffer);

    View4u8 make_view_4(u32 width, u32 height, Buffer8& buffer);
}


/* sub_view */

namespace image
{
    template <typename T, u32 C>
    inline ChannelSubView2D<T, C> sub_view(ChannelMatrix2D<T, C> const& view, Rect2Du32 const& range)
    {
        ChannelSubView2D<T, C> sub_view{};

        sub_view.channel_data = view.channel_data;
        sub_view.channel_width = view.width;
        sub_view.x_begin = range.x_begin;
        sub_view.y_begin = range.y_begin;
        sub_view.width = range.x_end - range.x_begin;
        sub_view.height = range.y_end - range.y_begin;

        return sub_view;
    }
}


/* map_rgba */

namespace image
{
    void map_rgba(ImageView const& src, View4u8 const& dst);

    void map_rgba(ImageView const& src, SubView4u8 const& dst);
    
    void map_rgba(View3u8 const& src, ImageView const& dst);
}