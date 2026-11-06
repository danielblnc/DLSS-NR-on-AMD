#include <stdarg.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wine/unixlib.h"
#include "bridge.h"

/* ntdll export that binds this PE to amdhip64_7.so (Wine unixlib ABI). */
extern NTSTATUS WINAPI bridge_init_unix_call(void) __asm__("__wine_init_unix_call");
static int ready = -1;

BOOL WINAPI DllMain(HINSTANCE h, DWORD r, void *v)
{
    (void)v;
    if (r == DLL_PROCESS_ATTACH) {
        struct init_params p = { -1 };
        DisableThreadLibraryCalls(h);
        if (bridge_init_unix_call()) return FALSE;
        WINE_UNIX_CALL(fn_init, &p);
        ready = p.status;
    }
    return TRUE;
}

void * WINAPI __hipRegisterFatBinary(const void * data)
{
    struct params_fn_hipRegisterFatBinary p;
    p.data = data;
    p.ret = NULL;
    if (ready != 0) return NULL;
    WINE_UNIX_CALL(fn_hipRegisterFatBinary, &p);
    return p.ret;
}

void WINAPI __hipUnregisterFatBinary(void * modules)
{
    struct params_fn_hipUnregisterFatBinary p;
    p.modules = modules;
    p.dummy = 0;
    if (ready != 0) return;
    WINE_UNIX_CALL(fn_hipUnregisterFatBinary, &p);
}

void WINAPI __hipRegisterFunction(void * modules, const void * hostFunction, char * deviceFunction, const char * deviceName, unsigned int threadLimit, void * tid, void * bid, void * blockDim, void * gridDim, int * wSize)
{
    struct params_fn_hipRegisterFunction p;
    p.modules = modules;
    p.hostFunction = hostFunction;
    p.deviceFunction = deviceFunction;
    p.deviceName = deviceName;
    p.threadLimit = threadLimit;
    p.tid = tid;
    p.bid = bid;
    p.blockDim = blockDim;
    p.gridDim = gridDim;
    p.wSize = wSize;
    p.dummy = 0;
    if (ready != 0) return;
    WINE_UNIX_CALL(fn_hipRegisterFunction, &p);
}

void WINAPI __hipRegisterVar(void * modules, void * var, char * hostVar, const char * deviceName, int ext, size_t size, int constant, int global)
{
    struct params_fn_hipRegisterVar p;
    p.modules = modules;
    p.var = var;
    p.hostVar = hostVar;
    p.deviceName = deviceName;
    p.ext = ext;
    p.size = size;
    p.constant = constant;
    p.global = global;
    p.dummy = 0;
    if (ready != 0) return;
    WINE_UNIX_CALL(fn_hipRegisterVar, &p);
}

int WINAPI hipCreateTextureObject(uint64_t * pTexObject, const void * pResDesc, const void * pTexDesc, const void * pResViewDesc)
{
    struct params_fn_hipCreateTextureObject p;
    p.pTexObject = pTexObject;
    p.pResDesc = pResDesc;
    p.pTexDesc = pTexDesc;
    p.pResViewDesc = pResViewDesc;
    p.ret = 100;
    if (ready != 0) return 100;
    WINE_UNIX_CALL(fn_hipCreateTextureObject, &p);
    return p.ret;
}

int WINAPI hipDestroyTextureObject(uint64_t textureObject)
{
    struct params_fn_hipDestroyTextureObject p;
    p.textureObject = textureObject;
    p.ret = 100;
    if (ready != 0) return 100;
    WINE_UNIX_CALL(fn_hipDestroyTextureObject, &p);
    return p.ret;
}

int WINAPI hipDestroyExternalMemory(void * extMem)
{
    struct params_fn_hipDestroyExternalMemory p;
    p.extMem = extMem;
    p.ret = 100;
    if (ready != 0) return 100;
    WINE_UNIX_CALL(fn_hipDestroyExternalMemory, &p);
    return p.ret;
}

int WINAPI hipImportExternalMemory(void ** extMem_out, const void * memHandleDesc)
{
    struct params_fn_hipImportExternalMemory p;
    p.extMem_out = extMem_out;
    p.memHandleDesc = memHandleDesc;
    p.ret = 100;
    if (ready != 0) return 100;
    WINE_UNIX_CALL(fn_hipImportExternalMemory, &p);
    return p.ret;
}

int WINAPI hipExternalMemoryGetMappedBuffer(void ** devPtr, void * extMem, const void * bufferDesc)
{
    struct params_fn_hipExternalMemoryGetMappedBuffer p;
    p.devPtr = devPtr;
    p.extMem = extMem;
    p.bufferDesc = bufferDesc;
    p.ret = 100;
    if (ready != 0) return 100;
    WINE_UNIX_CALL(fn_hipExternalMemoryGetMappedBuffer, &p);
    return p.ret;
}

int WINAPI hipDeviceGetStreamPriorityRange(int * leastPriority, int * greatestPriority)
{
    struct params_fn_hipDeviceGetStreamPriorityRange p;
    p.leastPriority = leastPriority;
    p.greatestPriority = greatestPriority;
    p.ret = 100;
    if (ready != 0) return 100;
    WINE_UNIX_CALL(fn_hipDeviceGetStreamPriorityRange, &p);
    return p.ret;
}

int WINAPI hipDeviceSynchronize(void)
{
    struct params_fn_hipDeviceSynchronize p;
    p.ret = 100;
    if (ready != 0) return 100;
    WINE_UNIX_CALL(fn_hipDeviceSynchronize, &p);
    return p.ret;
}

int WINAPI hipDriverGetVersion(int * driverVersion)
{
    struct params_fn_hipDriverGetVersion p;
    p.driverVersion = driverVersion;
    p.ret = 100;
    if (ready != 0) return 100;
    WINE_UNIX_CALL(fn_hipDriverGetVersion, &p);
    return p.ret;
}

int WINAPI hipRuntimeGetVersion(int * runtimeVersion)
{
    struct params_fn_hipRuntimeGetVersion p;
    p.runtimeVersion = runtimeVersion;
    p.ret = 100;
    if (ready != 0) return 100;
    WINE_UNIX_CALL(fn_hipRuntimeGetVersion, &p);
    return p.ret;
}

int WINAPI hipEventCreate(void ** event)
{
    struct params_fn_hipEventCreate p;
    p.event = event;
    p.ret = 100;
    if (ready != 0) return 100;
    WINE_UNIX_CALL(fn_hipEventCreate, &p);
    return p.ret;
}

int WINAPI hipEventCreateWithFlags(void ** event, unsigned int flags)
{
    struct params_fn_hipEventCreateWithFlags p;
    p.event = event;
    p.flags = flags;
    p.ret = 100;
    if (ready != 0) return 100;
    WINE_UNIX_CALL(fn_hipEventCreateWithFlags, &p);
    return p.ret;
}

int WINAPI hipEventDestroy(void * event)
{
    struct params_fn_hipEventDestroy p;
    p.event = event;
    p.ret = 100;
    if (ready != 0) return 100;
    WINE_UNIX_CALL(fn_hipEventDestroy, &p);
    return p.ret;
}

int WINAPI hipEventElapsedTime(float * ms, void * start, void * stop)
{
    struct params_fn_hipEventElapsedTime p;
    p.ms = ms;
    p.start = start;
    p.stop = stop;
    p.ret = 100;
    if (ready != 0) return 100;
    WINE_UNIX_CALL(fn_hipEventElapsedTime, &p);
    return p.ret;
}

int WINAPI hipEventQuery(void * event)
{
    struct params_fn_hipEventQuery p;
    p.event = event;
    p.ret = 100;
    if (ready != 0) return 100;
    WINE_UNIX_CALL(fn_hipEventQuery, &p);
    return p.ret;
}

int WINAPI hipEventRecord(void * event, void * stream)
{
    struct params_fn_hipEventRecord p;
    p.event = event;
    p.stream = stream;
    p.ret = 100;
    if (ready != 0) return 100;
    WINE_UNIX_CALL(fn_hipEventRecord, &p);
    return p.ret;
}

int WINAPI hipEventSynchronize(void * event)
{
    struct params_fn_hipEventSynchronize p;
    p.event = event;
    p.ret = 100;
    if (ready != 0) return 100;
    WINE_UNIX_CALL(fn_hipEventSynchronize, &p);
    return p.ret;
}

int WINAPI hipFree(void * ptr)
{
    struct params_fn_hipFree p;
    p.ptr = ptr;
    p.ret = 100;
    if (ready != 0) return 100;
    WINE_UNIX_CALL(fn_hipFree, &p);
    return p.ret;
}

int WINAPI hipGetDevice(int * deviceId)
{
    struct params_fn_hipGetDevice p;
    p.deviceId = deviceId;
    p.ret = 100;
    if (ready != 0) return 100;
    WINE_UNIX_CALL(fn_hipGetDevice, &p);
    return p.ret;
}

int WINAPI hipGetDeviceCount(int * count)
{
    struct params_fn_hipGetDeviceCount p;
    p.count = count;
    p.ret = 100;
    if (ready != 0) return 100;
    WINE_UNIX_CALL(fn_hipGetDeviceCount, &p);
    return p.ret;
}

int WINAPI hipGetDevicePropertiesR0600(void * prop, int device)
{
    struct params_fn_hipGetDevicePropertiesR0600 p;
    p.prop = prop;
    p.device = device;
    p.ret = 100;
    if (ready != 0) return 100;
    WINE_UNIX_CALL(fn_hipGetDevicePropertiesR0600, &p);
    return p.ret;
}

const char * WINAPI hipGetErrorString(int hipError)
{
    struct params_fn_hipGetErrorString p;
    p.hipError = hipError;
    p.ret = "hipErrorNotInitialized";
    if (ready != 0) return p.ret;
    WINE_UNIX_CALL(fn_hipGetErrorString, &p);
    return p.ret;
}

int WINAPI hipGetLastError(void)
{
    struct params_fn_hipGetLastError p;
    p.ret = 100;
    if (ready != 0) return 100;
    WINE_UNIX_CALL(fn_hipGetLastError, &p);
    return p.ret;
}

int WINAPI hipHostFree(void * ptr)
{
    struct params_fn_hipHostFree p;
    p.ptr = ptr;
    p.ret = 100;
    if (ready != 0) return 100;
    WINE_UNIX_CALL(fn_hipHostFree, &p);
    return p.ret;
}

int WINAPI hipHostGetDevicePointer(void ** devPtr, void * hstPtr, unsigned int flags)
{
    struct params_fn_hipHostGetDevicePointer p;
    p.devPtr = devPtr;
    p.hstPtr = hstPtr;
    p.flags = flags;
    p.ret = 100;
    if (ready != 0) return 100;
    WINE_UNIX_CALL(fn_hipHostGetDevicePointer, &p);
    return p.ret;
}

int WINAPI hipHostMalloc(void ** ptr, size_t size, unsigned int flags)
{
    struct params_fn_hipHostMalloc p;
    p.ptr = ptr;
    p.size = size;
    p.flags = flags;
    p.ret = 100;
    if (ready != 0) return 100;
    WINE_UNIX_CALL(fn_hipHostMalloc, &p);
    return p.ret;
}

int WINAPI hipLaunchKernel(const void * function_address, const struct hip_dim3 * numBlocks, const struct hip_dim3 * dimBlocks, void ** args, size_t sharedMemBytes, void * stream)
{
    struct params_fn_hipLaunchKernel p;
    p.function_address = function_address;
    if (numBlocks) p.numBlocks = *numBlocks;
    else { p.numBlocks.x = 1; p.numBlocks.y = 1; p.numBlocks.z = 1; }
    if (dimBlocks) p.dimBlocks = *dimBlocks;
    else { p.dimBlocks.x = 1; p.dimBlocks.y = 1; p.dimBlocks.z = 1; }
    p.args = args;
    p.sharedMemBytes = sharedMemBytes;
    p.stream = stream;
    p.ret = 100;
    if (ready != 0) return 100;
    WINE_UNIX_CALL(fn_hipLaunchKernel, &p);
    return p.ret;
}

int WINAPI hipMalloc(void ** ptr, size_t size)
{
    struct params_fn_hipMalloc p;
    p.ptr = ptr;
    p.size = size;
    p.ret = 100;
    if (ready != 0) return 100;
    WINE_UNIX_CALL(fn_hipMalloc, &p);
    return p.ret;
}

int WINAPI hipMallocPitch(void ** ptr, size_t * pitch, size_t width, size_t height)
{
    struct params_fn_hipMallocPitch p;
    p.ptr = ptr;
    p.pitch = pitch;
    p.width = width;
    p.height = height;
    p.ret = 100;
    if (ready != 0) return 100;
    WINE_UNIX_CALL(fn_hipMallocPitch, &p);
    return p.ret;
}

int WINAPI hipMemcpy(void * dst, const void * src, size_t sizeBytes, int kind)
{
    struct params_fn_hipMemcpy p;
    p.dst = dst;
    p.src = src;
    p.sizeBytes = sizeBytes;
    p.kind = kind;
    p.ret = 100;
    if (ready != 0) return 100;
    WINE_UNIX_CALL(fn_hipMemcpy, &p);
    return p.ret;
}

int WINAPI hipMemcpy2D(void * dst, size_t dpitch, const void * src, size_t spitch, size_t width, size_t height, int kind)
{
    struct params_fn_hipMemcpy2D p;
    p.dst = dst;
    p.dpitch = dpitch;
    p.src = src;
    p.spitch = spitch;
    p.width = width;
    p.height = height;
    p.kind = kind;
    p.ret = 100;
    if (ready != 0) return 100;
    WINE_UNIX_CALL(fn_hipMemcpy2D, &p);
    return p.ret;
}

int WINAPI hipMemcpyAsync(void * dst, const void * src, size_t sizeBytes, int kind, void * stream)
{
    struct params_fn_hipMemcpyAsync p;
    p.dst = dst;
    p.src = src;
    p.sizeBytes = sizeBytes;
    p.kind = kind;
    p.stream = stream;
    p.ret = 100;
    if (ready != 0) return 100;
    WINE_UNIX_CALL(fn_hipMemcpyAsync, &p);
    return p.ret;
}

int WINAPI hipMemcpyFromSymbol(void * dst, const void * symbol, size_t sizeBytes, size_t offset, int kind)
{
    struct params_fn_hipMemcpyFromSymbol p;
    p.dst = dst;
    p.symbol = symbol;
    p.sizeBytes = sizeBytes;
    p.offset = offset;
    p.kind = kind;
    p.ret = 100;
    if (ready != 0) return 100;
    WINE_UNIX_CALL(fn_hipMemcpyFromSymbol, &p);
    return p.ret;
}

int WINAPI hipMemcpyToSymbol(const void * symbol, const void * src, size_t sizeBytes, size_t offset, int kind)
{
    struct params_fn_hipMemcpyToSymbol p;
    p.symbol = symbol;
    p.src = src;
    p.sizeBytes = sizeBytes;
    p.offset = offset;
    p.kind = kind;
    p.ret = 100;
    if (ready != 0) return 100;
    WINE_UNIX_CALL(fn_hipMemcpyToSymbol, &p);
    return p.ret;
}

int WINAPI hipMemset(void * dst, int value, size_t sizeBytes)
{
    struct params_fn_hipMemset p;
    p.dst = dst;
    p.value = value;
    p.sizeBytes = sizeBytes;
    p.ret = 100;
    if (ready != 0) return 100;
    WINE_UNIX_CALL(fn_hipMemset, &p);
    return p.ret;
}

int WINAPI hipMemset2D(void * dst, size_t pitch, int value, size_t width, size_t height)
{
    struct params_fn_hipMemset2D p;
    p.dst = dst;
    p.pitch = pitch;
    p.value = value;
    p.width = width;
    p.height = height;
    p.ret = 100;
    if (ready != 0) return 100;
    WINE_UNIX_CALL(fn_hipMemset2D, &p);
    return p.ret;
}

int WINAPI hipMemset2DAsync(void * dst, size_t pitch, int value, size_t width, size_t height, void * stream)
{
    struct params_fn_hipMemset2DAsync p;
    p.dst = dst;
    p.pitch = pitch;
    p.value = value;
    p.width = width;
    p.height = height;
    p.stream = stream;
    p.ret = 100;
    if (ready != 0) return 100;
    WINE_UNIX_CALL(fn_hipMemset2DAsync, &p);
    return p.ret;
}

int WINAPI hipMemsetAsync(void * dst, int value, size_t sizeBytes, void * stream)
{
    struct params_fn_hipMemsetAsync p;
    p.dst = dst;
    p.value = value;
    p.sizeBytes = sizeBytes;
    p.stream = stream;
    p.ret = 100;
    if (ready != 0) return 100;
    WINE_UNIX_CALL(fn_hipMemsetAsync, &p);
    return p.ret;
}

int WINAPI hipMemsetD16Async(void * dst, unsigned short value, size_t count, void * stream)
{
    struct params_fn_hipMemsetD16Async p;
    p.dst = dst;
    p.value = value;
    p.count = count;
    p.stream = stream;
    p.ret = 100;
    if (ready != 0) return 100;
    WINE_UNIX_CALL(fn_hipMemsetD16Async, &p);
    return p.ret;
}

int WINAPI hipOccupancyMaxActiveBlocksPerMultiprocessor(int * numBlocks, const void * f, int blockSize, size_t dynSharedMemPerBlk)
{
    struct params_fn_hipOccupancyMaxActiveBlocksPerMultiprocessor p;
    p.numBlocks = numBlocks;
    p.f = f;
    p.blockSize = blockSize;
    p.dynSharedMemPerBlk = dynSharedMemPerBlk;
    p.ret = 100;
    if (ready != 0) return 100;
    WINE_UNIX_CALL(fn_hipOccupancyMaxActiveBlocksPerMultiprocessor, &p);
    return p.ret;
}

int WINAPI hipSetDevice(int deviceId)
{
    struct params_fn_hipSetDevice p;
    p.deviceId = deviceId;
    p.ret = 100;
    if (ready != 0) return 100;
    WINE_UNIX_CALL(fn_hipSetDevice, &p);
    return p.ret;
}

int WINAPI hipStreamCreate(void ** stream)
{
    struct params_fn_hipStreamCreate p;
    p.stream = stream;
    p.ret = 100;
    if (ready != 0) return 100;
    WINE_UNIX_CALL(fn_hipStreamCreate, &p);
    return p.ret;
}

int WINAPI hipStreamCreateWithFlags(void ** stream, unsigned int flags)
{
    struct params_fn_hipStreamCreateWithFlags p;
    p.stream = stream;
    p.flags = flags;
    p.ret = 100;
    if (ready != 0) return 100;
    WINE_UNIX_CALL(fn_hipStreamCreateWithFlags, &p);
    return p.ret;
}

int WINAPI hipStreamCreateWithPriority(void ** stream, unsigned int flags, int priority)
{
    struct params_fn_hipStreamCreateWithPriority p;
    p.stream = stream;
    p.flags = flags;
    p.priority = priority;
    p.ret = 100;
    if (ready != 0) return 100;
    WINE_UNIX_CALL(fn_hipStreamCreateWithPriority, &p);
    return p.ret;
}

int WINAPI hipStreamDestroy(void * stream)
{
    struct params_fn_hipStreamDestroy p;
    p.stream = stream;
    p.ret = 100;
    if (ready != 0) return 100;
    WINE_UNIX_CALL(fn_hipStreamDestroy, &p);
    return p.ret;
}

int WINAPI hipStreamSynchronize(void * stream)
{
    struct params_fn_hipStreamSynchronize p;
    p.stream = stream;
    p.ret = 100;
    if (ready != 0) return 100;
    WINE_UNIX_CALL(fn_hipStreamSynchronize, &p);
    return p.ret;
}
