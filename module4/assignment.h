#pragma once

#ifndef ASSIGNMENT_H
#define ASSIGNMENT_H

#include "cuda_runtime.h"
#include <stdio.h>
#include <cstdlib>
#include <iostream>


// Helper function and #DEFINE to help with error checking (found from stackoverflow)
#define gpuErrchk(ans) { gpuAssert((ans), __FILE__, __LINE__); }
inline void gpuAssert(cudaError_t code, const char* file, int line, bool abort = true)
{
    if (code != cudaSuccess)
    {
        fprintf(stderr, "GPUassert: %s %s %d\n", cudaGetErrorString(code), file, line);
        if (abort) exit(code);
    }
}

// https://developer.nvidia.com/blog/how-optimize-data-transfers-cuda-cc/
// Convenience function for checking CUDA runtime API results
// can be wrapped around any runtime API call. No-op in release builds.
inline
cudaError_t checkCuda(cudaError_t result)
{
#if defined(DEBUG) || defined(_DEBUG)
    if (result != cudaSuccess) {
        fprintf(stderr, "CUDA Runtime Error: %s\n",
            cudaGetErrorString(result));
        //assert(result == cudaSuccess);
    }
#endif
    return result;
}

class TimeCodeBlock
{
    // events for timing
    cudaEvent_t startEvent, stopEvent;

    const char* name;
public:
    TimeCodeBlock(const char* blockName) : name(blockName) {
        gpuErrchk(cudaEventCreate(&startEvent));
        gpuErrchk(cudaEventCreate(&stopEvent));

        gpuErrchk(cudaEventRecord(startEvent, 0));
    }

    ~TimeCodeBlock() {
        gpuErrchk(cudaEventRecord(stopEvent, 0));
        gpuErrchk(cudaEventSynchronize(stopEvent));

        float time_ms;
        gpuErrchk(cudaEventElapsedTime(&time_ms, startEvent, stopEvent));

        std::cout << name << " Execution time = " << time_ms << " milliseconds." << std::endl;

        gpuErrchk(cudaEventDestroy(startEvent));
        gpuErrchk(cudaEventDestroy(stopEvent));
    }
};


template <typename T>
class HostMemory
{
    bool pinnedMemory;
    T* host_ptr;
    size_t _size;

public:

    HostMemory() : pinnedMemory(false), _size(-1) {
    }

    void allocate(size_t bufferCount, bool _pinnedMemory = false) {
        pinnedMemory = _pinnedMemory;
        _size = bufferCount;
        size_t bytes = _size * sizeof(T);
        if (pinnedMemory) {
            gpuErrchk(cudaMallocHost((void**)&host_ptr, bytes));
        }
        else {
            host_ptr = (T*)malloc(bytes);
        }
    }

    ~HostMemory() {
        if (pinnedMemory) {
            gpuErrchk(cudaFreeHost(host_ptr));
        }
        else {
            free(host_ptr);
        }
    }

    inline T* ptr() const { return host_ptr; }
    inline size_t size() const { return _size; }
};

template <typename T>
class DeviceMemory
{
    T* d_ptr;
    size_t _size;

public:
    DeviceMemory(size_t bufferCount) : _size(bufferCount) {
        allocate(bufferCount);
    }

    void allocate(size_t bufferCount) {
        _size = bufferCount;
        size_t bytes = _size * sizeof(T);
        gpuErrchk(cudaMalloc((void**)&d_ptr, bytes));
    }

    ~DeviceMemory() {
        cudaFree(d_ptr);
    }

    inline T* ptr() const { return d_ptr; }
    inline size_t size() const { return _size; }
};

#endif

