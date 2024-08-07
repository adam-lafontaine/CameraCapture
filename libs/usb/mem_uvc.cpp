#pragma once

#include <cstdlib>
#include <string.h>


namespace mem_uvc
{
    static u32 malloc_count = 0;
    static u32 realloc_count = 0;
    static u32 free_count = 0;

    static u32 alloc_count = 0;

    static u32 alloc_bytes = 0;

    class MemoryTag
    {
    public:
        u32 bytes = 0;
        cstr tag = 0;
    };


    Stats get_stats()
    {
        Stats s{};

        s.malloc = malloc_count;
        s.realloc = realloc_count;
        s.free = free_count;

        s.count = alloc_count;
        s.bytes = alloc_bytes;

        return s;
    }
}


#ifndef LIBUVC_TRACK_MEMORY

namespace mem_uvc
{
    void* malloc(u32 n_elements, u32 element_size, cstr tag)
    {
        return std::calloc(n_elements, element_size);
    }


    void* realloc(void* ptr, u32 n_elements, u32 element_size)
    {
        return std::realloc(ptr, n_elements * element_size);
    }


    char* str_dup(cstr str, cstr tag)
    {
#ifdef _WIN32

        return _strdup(str);

#else

        return strdup(str);

#endif // _WIN32
    }


    void free(void* ptr)
    {
        std::free(ptr);
    }
}

#else


#include <unordered_map>


namespace mem_uvc
{
    static std::unordered_map<u64, MemoryTag> ptr_tags;



    void* malloc(u32 n_elements, u32 element_size, cstr tag)
    {       
        alloc_count++;
        auto bytes = n_elements * element_size;
        alloc_bytes += bytes;

        malloc_count++;
        auto data = std::calloc(n_elements, element_size);
        ptr_tags[(u64)data] = { bytes, tag };

        return data;
    }


    void* realloc(void* ptr, u32 n_elements, u32 element_size)
    {      
        auto& tag = ptr_tags[(u64)ptr];
        auto bytes = tag.bytes;
        auto new_bytes = n_elements * element_size;

        if (new_bytes <= bytes)
        {
            return ptr;
        }

        alloc_bytes -= bytes;
        alloc_bytes += new_bytes;

        realloc_count++;
        auto data = std::realloc(ptr, new_bytes);
        ptr_tags[(u64)data] = { new_bytes, tag.tag };

        return data;
    }


    char* str_dup(cstr str, cstr tag)
    {
        alloc_count++;
        char* data = 0;

#ifdef _WIN32

        data = _strdup(str);

#else

        data = strdup(str);

#endif // _WIN32

        
        auto bytes = (u32)strlen(str) + 1;
        ptr_tags[(u64)data] = { bytes, tag };

        return data;
    }


    void free(void* ptr)
    {        
        alloc_count--;

        auto bytes = ptr_tags[(u64)ptr].bytes;
        alloc_bytes -= bytes;

        free_count++;
        std::free(ptr);
    }
}

#endif // LIBUVC_TRACK_MEMORY