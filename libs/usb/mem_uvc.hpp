#pragma once


namespace mem_uvc
{
    void* malloc(u32 n_elements, u32 element_size, cstr tag);

    void* realloc(void* ptr, u32 n_elements, u32 element_size);

    void free(void* ptr);    
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


    inline void uvc_free_string(cstr str)
    {
        std::free((void*)str);
    }
}