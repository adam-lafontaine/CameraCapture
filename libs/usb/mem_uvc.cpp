#pragma once

#include <cstdlib>
#include <unordered_map>


namespace mem_uvc
{
    static u32 malloc_count = 0;
    static u32 realloc_count = 0;
    static u32 free_count = 0;

    static u32 alloc_count = 0;

    static u32 alloc_bytes = 0;

    static std::unordered_map<u64, u32> ptr_bytes;


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


namespace mem_uvc
{
    void* malloc(u32 n_elements, u32 element_size, cstr tag)
    {       
        alloc_count++;
        auto bytes = n_elements * element_size;
        alloc_bytes += bytes;

        malloc_count++;
        auto data = std::calloc(n_elements, element_size);
        ptr_bytes[(u64)data] = bytes;

        return data;
    }


    void* realloc(void* ptr, u32 n_elements, u32 element_size)
    {        
        auto bytes = ptr_bytes[(u64)ptr];
        auto new_bytes = n_elements * element_size;

        if (new_bytes <= bytes)
        {
            return ptr;
        }

        alloc_bytes -= bytes;
        alloc_bytes += new_bytes;

        realloc_count++;
        auto data = std::realloc(ptr, new_bytes);
        ptr_bytes[(u64)data] = new_bytes;

        return data;
    }


    void free(void* ptr)
    {        
        alloc_count--;

        auto bytes = ptr_bytes[(u64)ptr];
        alloc_bytes -= bytes;

        free_count++;
        std::free(ptr);
    }
}