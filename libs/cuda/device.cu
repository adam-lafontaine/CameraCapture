#include "device.hpp"
#include "cuda_def.cuh"

#include <cassert>
#include <cstddef>

#ifdef CUDA_PRINT_ERROR

#include <cstdio>
#include <cstring>

#endif


static void check_error(cudaError_t err, cstr label = "")
{
    if (err == cudaSuccess)
    {
        return;
    }

    #ifdef CUDA_PRINT_ERROR
    #ifndef	NDEBUG

    printf("\n*** CUDA ERROR ***\n\n");
    printf("%s", cudaGetErrorString(err));

    if (std::strlen(label))
    {
        printf("\n%s", label);
    }
    
    printf("\n\n******************\n\n");

    #endif
    #endif
}


namespace cuda
{
    u8* device_malloc(size_t n_bytes)
    {
        assert(n_bytes);

        u8* data;

        auto err = cudaMalloc((void**)&(data), n_bytes);
        check_error(err, "malloc");

        if (err != cudaSuccess)
        {
            return nullptr;
        }

        return data;
    }


    u8* unified_malloc(size_t n_bytes)
    {
        assert(n_bytes);

        u8* data;

        auto err = cudaMallocManaged((void**)&(data), n_bytes);
        check_error(err, "malloc");

        if (err != cudaSuccess)
        {
            return nullptr;
        }

        return data;
    }


    bool device_free(void* data)
    {
        if (data)
        {
            auto err = cudaFree(data);
            check_error(err, "free");

            return err == cudaSuccess;
        }

        return true;
    }
    

    bool memcpy_to_device(const void* host_src, void* device_dst, size_t n_bytes)
    {
        cudaError_t err = cudaMemcpy(device_dst, host_src, n_bytes, cudaMemcpyHostToDevice);
        check_error(err, "memcpy_to_device");

        return err == cudaSuccess;
    }


    bool memcpy_to_host(const void* device_src, void* host_dst, size_t n_bytes)
    {
        cudaError_t err = cudaMemcpy(host_dst, device_src, n_bytes, cudaMemcpyDeviceToHost);
        check_error(err, "memcpy_to_host");

        return err == cudaSuccess;
    }


    bool no_errors(cstr label)
    {
        #ifndef	NDEBUG

        cudaError_t err = cudaGetLastError();
        check_error(err, label);

        return err == cudaSuccess;

        #else

        return true;

        #endif
    }


    bool launch_success(cstr label)
    {
        #ifndef	NDEBUG

        cudaError_t err = cudaDeviceSynchronize();
        check_error(err, label);

        return err == cudaSuccess;

        #else

        return true;

        #endif
    }
}