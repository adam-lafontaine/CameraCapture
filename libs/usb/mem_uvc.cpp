#pragma once

#include <cstdlib>


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


    void free(void* ptr)
    {
        std::free(ptr);
    }
}