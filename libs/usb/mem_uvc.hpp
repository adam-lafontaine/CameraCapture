#pragma once

#ifndef __linux__
#error Linux only
#endif

namespace mem_uvc
{
    void* malloc(u32 n_elements, u32 element_size, cstr tag);

    void* realloc(void* ptr, u32 n_elements, u32 element_size);

    char* str_dup(cstr str, cstr tag);

    void free(void* ptr);


    class Stats
    {
    public:

        u32 malloc = 0;
        u32 realloc = 0;
        u32 free = 0;

        u32 count = 0;
        u32 bytes = 0;
    };


    Stats get_stats();
}


namespace uvc
{
    template <typename T>
    inline T* uvc_malloc(cstr tag)
    {
        return (T*)mem_uvc::malloc(1, sizeof(T), tag);
    }


    template <typename T>
    inline T* uvc_malloc(u32 n_elements, cstr tag)
    {
        return (T*)mem_uvc::malloc(n_elements, sizeof(T), tag);
    }


    template <typename T>
    inline T* uvc_realloc(T* ptr, u32 n_elements)
    {
        return (T*)mem_uvc::realloc((void*)ptr, n_elements, sizeof(T));
    }


    template <typename T>
    inline void uvc_free(T* ptr)
    {
        mem_uvc::free(ptr);
    }


    inline char* uvc_strdup(cstr str, cstr tag)
    {
        return mem_uvc::str_dup(str, tag);
    }


    inline void uvc_free_string(cstr str)
    {
        mem_uvc::free((void*)str);
    }
}