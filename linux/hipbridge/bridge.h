#ifndef HIPBRIDGE_H
#define HIPBRIDGE_H

#include <stddef.h>
#include <stdint.h>

struct hip_dim3 {
    uint32_t x;
    uint32_t y;
    uint32_t z;
};

enum {
    fn_init,
    fn_hipRegisterFatBinary,
    fn_hipUnregisterFatBinary,
    fn_hipRegisterFunction,
    fn_hipRegisterVar,
    fn_hipCreateTextureObject,
    fn_hipDestroyTextureObject,
    fn_hipDestroyExternalMemory,
    fn_hipImportExternalMemory,
    fn_hipExternalMemoryGetMappedBuffer,
    fn_hipDeviceGetStreamPriorityRange,
    fn_hipDeviceSynchronize,
    fn_hipDriverGetVersion,
    fn_hipRuntimeGetVersion,
    fn_hipEventCreate,
    fn_hipEventCreateWithFlags,
    fn_hipEventDestroy,
    fn_hipEventElapsedTime,
    fn_hipEventQuery,
    fn_hipEventRecord,
    fn_hipEventSynchronize,
    fn_hipFree,
    fn_hipGetDevice,
    fn_hipGetDeviceCount,
    fn_hipGetDevicePropertiesR0600,
    fn_hipGetErrorString,
    fn_hipGetLastError,
    fn_hipHostFree,
    fn_hipHostGetDevicePointer,
    fn_hipHostMalloc,
    fn_hipLaunchKernel,
    fn_hipMalloc,
    fn_hipMallocPitch,
    fn_hipMemcpy,
    fn_hipMemcpy2D,
    fn_hipMemcpyAsync,
    fn_hipMemcpyFromSymbol,
    fn_hipMemcpyToSymbol,
    fn_hipMemset,
    fn_hipMemset2D,
    fn_hipMemset2DAsync,
    fn_hipMemsetAsync,
    fn_hipMemsetD16Async,
    fn_hipOccupancyMaxActiveBlocksPerMultiprocessor,
    fn_hipSetDevice,
    fn_hipStreamCreate,
    fn_hipStreamCreateWithFlags,
    fn_hipStreamCreateWithPriority,
    fn_hipStreamDestroy,
    fn_hipStreamSynchronize,
    fn_count
};

struct init_params {
    int status;
};

struct params_fn_hipRegisterFatBinary {
    const void * data;
    void * ret;
};

struct params_fn_hipUnregisterFatBinary {
    void * modules;
    int dummy;
};

struct params_fn_hipRegisterFunction {
    void * modules;
    const void * hostFunction;
    char * deviceFunction;
    const char * deviceName;
    unsigned int threadLimit;
    void * tid;
    void * bid;
    void * blockDim;
    void * gridDim;
    int * wSize;
    int dummy;
};

struct params_fn_hipRegisterVar {
    void * modules;
    void * var;
    char * hostVar;
    const char * deviceName;
    int ext;
    size_t size;
    int constant;
    int global;
    int dummy;
};

struct params_fn_hipCreateTextureObject {
    uint64_t * pTexObject;
    const void * pResDesc;
    const void * pTexDesc;
    const void * pResViewDesc;
    int ret;
};

struct params_fn_hipDestroyTextureObject {
    uint64_t textureObject;
    int ret;
};

struct params_fn_hipDestroyExternalMemory {
    void * extMem;
    int ret;
};

struct params_fn_hipImportExternalMemory {
    void ** extMem_out;
    const void * memHandleDesc;
    int ret;
};

struct params_fn_hipExternalMemoryGetMappedBuffer {
    void ** devPtr;
    void * extMem;
    const void * bufferDesc;
    int ret;
};

struct params_fn_hipDeviceGetStreamPriorityRange {
    int * leastPriority;
    int * greatestPriority;
    int ret;
};

struct params_fn_hipDeviceSynchronize {
    int ret;
};

struct params_fn_hipDriverGetVersion {
    int * driverVersion;
    int ret;
};

struct params_fn_hipRuntimeGetVersion {
    int * runtimeVersion;
    int ret;
};

struct params_fn_hipEventCreate {
    void ** event;
    int ret;
};

struct params_fn_hipEventCreateWithFlags {
    void ** event;
    unsigned int flags;
    int ret;
};

struct params_fn_hipEventDestroy {
    void * event;
    int ret;
};

struct params_fn_hipEventElapsedTime {
    float * ms;
    void * start;
    void * stop;
    int ret;
};

struct params_fn_hipEventQuery {
    void * event;
    int ret;
};

struct params_fn_hipEventRecord {
    void * event;
    void * stream;
    int ret;
};

struct params_fn_hipEventSynchronize {
    void * event;
    int ret;
};

struct params_fn_hipFree {
    void * ptr;
    int ret;
};

struct params_fn_hipGetDevice {
    int * deviceId;
    int ret;
};

struct params_fn_hipGetDeviceCount {
    int * count;
    int ret;
};

struct params_fn_hipGetDevicePropertiesR0600 {
    void * prop;
    int device;
    int ret;
};

struct params_fn_hipGetErrorString {
    int hipError;
    const char * ret;
};

struct params_fn_hipGetLastError {
    int ret;
};

struct params_fn_hipHostFree {
    void * ptr;
    int ret;
};

struct params_fn_hipHostGetDevicePointer {
    void ** devPtr;
    void * hstPtr;
    unsigned int flags;
    int ret;
};

struct params_fn_hipHostMalloc {
    void ** ptr;
    size_t size;
    unsigned int flags;
    int ret;
};

struct params_fn_hipLaunchKernel {
    const void * function_address;
    struct hip_dim3 numBlocks;
    struct hip_dim3 dimBlocks;
    void ** args;
    size_t sharedMemBytes;
    void * stream;
    int ret;
};

struct params_fn_hipMalloc {
    void ** ptr;
    size_t size;
    int ret;
};

struct params_fn_hipMallocPitch {
    void ** ptr;
    size_t * pitch;
    size_t width;
    size_t height;
    int ret;
};

struct params_fn_hipMemcpy {
    void * dst;
    const void * src;
    size_t sizeBytes;
    int kind;
    int ret;
};

struct params_fn_hipMemcpy2D {
    void * dst;
    size_t dpitch;
    const void * src;
    size_t spitch;
    size_t width;
    size_t height;
    int kind;
    int ret;
};

struct params_fn_hipMemcpyAsync {
    void * dst;
    const void * src;
    size_t sizeBytes;
    int kind;
    void * stream;
    int ret;
};

struct params_fn_hipMemcpyFromSymbol {
    void * dst;
    const void * symbol;
    size_t sizeBytes;
    size_t offset;
    int kind;
    int ret;
};

struct params_fn_hipMemcpyToSymbol {
    const void * symbol;
    const void * src;
    size_t sizeBytes;
    size_t offset;
    int kind;
    int ret;
};

struct params_fn_hipMemset {
    void * dst;
    int value;
    size_t sizeBytes;
    int ret;
};

struct params_fn_hipMemset2D {
    void * dst;
    size_t pitch;
    int value;
    size_t width;
    size_t height;
    int ret;
};

struct params_fn_hipMemset2DAsync {
    void * dst;
    size_t pitch;
    int value;
    size_t width;
    size_t height;
    void * stream;
    int ret;
};

struct params_fn_hipMemsetAsync {
    void * dst;
    int value;
    size_t sizeBytes;
    void * stream;
    int ret;
};

struct params_fn_hipMemsetD16Async {
    void * dst;
    unsigned short value;
    size_t count;
    void * stream;
    int ret;
};

struct params_fn_hipOccupancyMaxActiveBlocksPerMultiprocessor {
    int * numBlocks;
    const void * f;
    int blockSize;
    size_t dynSharedMemPerBlk;
    int ret;
};

struct params_fn_hipSetDevice {
    int deviceId;
    int ret;
};

struct params_fn_hipStreamCreate {
    void ** stream;
    int ret;
};

struct params_fn_hipStreamCreateWithFlags {
    void ** stream;
    unsigned int flags;
    int ret;
};

struct params_fn_hipStreamCreateWithPriority {
    void ** stream;
    unsigned int flags;
    int priority;
    int ret;
};

struct params_fn_hipStreamDestroy {
    void * stream;
    int ret;
};

struct params_fn_hipStreamSynchronize {
    void * stream;
    int ret;
};

#endif
