#pragma once

#include "../util/memory_buffer.hpp"
#include "../util/stack_buffer.hpp"
#include "../qsprintf/qsprintf.hpp"
#include "../util/numeric.hpp"

namespace mb = memory_buffer;
namespace sb = stack_buffer;
namespace num = numeric;


template <typename T>
class SpanView
{
public:
	T* begin = nullptr;
	u32 length = 0;
};


class StringView
{
public:
    char* begin = nullptr;
    u32 capacity = 0;
    u32 length = 0;
};


using ByteView = SpanView<u8>;


namespace span
{
    template <typename T>
    inline SpanView<T> make_view(MemoryBuffer<T>& buffer)
    {
        SpanView<T> view{};

        view.begin = buffer.data_;
        view.length = buffer.capacity_;

        return view;
    }


    template <typename T>
    inline SpanView<T> push_span(MemoryBuffer<T>& buffer, u32 length)
    {
        SpanView<T> view{};

        auto data = mb::push_elements(buffer, length);
        if (data)
        {
            view.begin = data;
            view.length = length;
        }

        return view;
    }


    template <typename T, u64 N>
    inline SpanView<T> push_span(StackBuffer<T, N>& buffer, u32 length)
    {
        SpanView<T> view{};

        auto data = sb::push_elements(buffer, length);
        if (data)
        {
            view.begin = data;
            view.length = length;
        }

        return view;
    }


    template <typename T>
    inline SpanView<T> make_view(T* data, u32 length)
    {
        SpanView<T> view{};

        view.begin = data;
        view.length = length;

        return view;
    }


    template <typename T>
    inline SpanView<T> sub_view(SpanView<T> view, u32 offset, u32 len)
    {
        SpanView<T> sub{};

        sub.begin = view.begin + offset;
        sub.length = len;

        return sub;
    }
}


namespace span
{
    void copy_u8(u8* src, u8* dst, u64 len);


    void fill_u8(u8* dst, u8 value, u64 len);


    void fill_u32(u32* dst, u32 value, u64 len);


    template <typename T>
    inline void copy_span(SpanView<T> const& src, SpanView<T> const& dst)
    {
        copy_u8((u8*)src.begin, (u8*)dst.begin, src.length * sizeof(T));
    }


    template <typename T>
    inline void fill_span_32(SpanView<T> const& dst, T value)
    {
        static_assert(sizeof(T) == sizeof(u32));
        auto val = *((u32*)&value);
        fill_u32((u32*)dst.begin, val, dst.length);
    }


    template <typename T>
    inline void fill_span_8(SpanView<T> const& dst, T value)
    {
        static_assert(sizeof(T) == sizeof(u8));

        auto val = *((u8*)&value);
        fill_u8((u8*)dst.begin, val, dst.length);
    }
    
    
    template <typename T>
	inline void fill_span(SpanView<T> const& dst, T value)
	{
        T* d = dst.begin;
		for (u32 i = 0; i < dst.length; ++i)
		{
			d[i] = value;
		}
	}
}


/* string view */

namespace span
{
    inline constexpr u32 strlen(cstr text)
    {
        u32 len = 0;

        for (; text[len]; len++) {}

        return len;
    }


    inline constexpr int strcmp(cstr str_a, cstr str_b)
    {
        while (*str_a && (*str_a == *str_b))
        {
            str_a++;
            str_b++;
        }

        return *(u8*)str_a - *(u8*)str_b;
    }


    inline cstr to_cstr(StringView const& view)
    {
        return (cstr)view.begin;
    }


    inline constexpr StringView to_string_view(cstr text)
    {
        StringView view{};

        view.begin = (char*)text;
        view.capacity = strlen(text);
        view.length = view.capacity;

        return view;
    }


    template <typename T>
    inline StringView to_string_view(T* begin, u32 length)
    {
        StringView view{};

        view.begin = (u8*)begin;
        view.length = length;
        view.capacity = length;

        return view;
    }


    inline void zero_string(StringView& view)
    {
        view.length = 0;

        fill_u8((u8*)view.begin, 0, view.capacity);
    }


    inline StringView make_view(u32 capacity, MemoryBuffer<u8>& buffer)
    {
        StringView view{};

        auto data = mb::push_elements(buffer, capacity);
        if (data)
        {
            view.begin = (char*)data;
            view.capacity = capacity;
            view.length = 0;
            
            zero_string(view);
        }

        return view;
    }


    inline StringView make_view(u32 capacity, char* buffer)
    {
        StringView view{};

        view.begin = buffer;
        view.capacity = capacity;
        view.length = 0;

        return view;
    }


    inline void set_length(StringView& view)
    {
        view.length = view.capacity;

        for (u32 i = 0; i < view.capacity; i++)
        {
            if (!view.begin[i])
            {
                view.length = i;
                return;
            }
        }
    }


    template <typename... VA_ARGS>
    inline void sprintf(StringView& view, cstr fmt, VA_ARGS... va_args)
    {
        view.length = (u32)qsnprintf(view.begin, (int)view.capacity, fmt, va_args...);
    }


    inline void copy_string(StringView const& src, StringView const& dst)
    {
        auto len = num::min(src.length, dst.length);
        
        copy_u8((u8*)src.begin, (u8*)dst.begin, len);
    }

}