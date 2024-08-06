#pragma once

#include <cstdlib>


namespace mem_uvc
{
    static u32 alloc_count = 0;
    static u32 alloc_bytes = 0;


    Stats get_stats()
    {
        Stats s{};

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
        //alloc_bytes += n_elements * element_size;

        return std::calloc(n_elements, element_size);
    }


    void* realloc(void* ptr, u32 n_elements, u32 element_size)
    {
        return std::realloc(ptr, n_elements * element_size);
    }


    void free(void* ptr)
    {
        alloc_count--;
        
        std::free(ptr);
    }
}