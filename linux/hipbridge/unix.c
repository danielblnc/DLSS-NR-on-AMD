#include <stdarg.h>
#include <dlfcn.h>
#include <elf.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include "winternl.h"
typedef NTSTATUS (*bridge_unix_fn_t)(void *);
#include "bridge.h"

static void *lib_hip;
static FILE *log_fp;

static void bridge_log(const char *fmt, ...)
{
    va_list ap;
    if (!log_fp) {
        log_fp = fopen("/home/dkp/.local/share/neuralhub/hipbridge.log", "a");
        (void)log_fp;
    }
    if (!log_fp) return;
    va_start(ap, fmt);
    vfprintf(log_fp, fmt, ap); fflush(log_fp);
    va_end(ap);
}

typedef unsigned int (*fn_d3dkmt_open_resource_t)(unsigned int, void *, unsigned int *, unsigned int *);
typedef int (*fn_d3dkmt_object_get_fd_t)(unsigned int);
typedef int (*fn_wine_server_handle_to_fd_t)(void *, unsigned int, int *, unsigned int *);

static fn_d3dkmt_open_resource_t p_d3dkmt_open_resource;
static fn_d3dkmt_object_get_fd_t p_d3dkmt_object_get_fd;
static fn_wine_server_handle_to_fd_t p_wine_server_handle_to_fd;
typedef int (*fn_vk_allocate_memory_t)(void *, const void *, const void *, void *);
static fn_vk_allocate_memory_t orig_vkAllocateMemory;


struct vk_mem_alloc_info {
    int sType;
    const void *pNext;
    uint64_t allocationSize;
    uint32_t memoryTypeIndex;
};

struct vk_export_mem_alloc_info {
    int sType;
    const void *pNext;
    uint32_t handleTypes;
    uint32_t pad;
};

static int hook_vkAllocateMemory(void *device, const void *pAllocateInfo, const void *pAllocator, void *pMemory)
{
    const struct vk_mem_alloc_info *in_info = (const struct vk_mem_alloc_info *)pAllocateInfo;
    if (in_info && in_info->sType == 5) {
        int can_export = 1;
        const struct vk_export_mem_alloc_info *cur = (const struct vk_export_mem_alloc_info *)in_info->pNext;
        while (cur) {
            if (cur->sType == 0x3b9be342 || cur->sType == 0x3b9be729 || cur->sType == 0x3b9d8150) {
                can_export = 0;
                break;
            }
            cur = (const struct vk_export_mem_alloc_info *)cur->pNext;
        }
        if (can_export) {
            struct vk_export_mem_alloc_info exp = { 0x3b9be342, in_info->pNext, 0x2, 0 };
            struct vk_mem_alloc_info copy = *in_info;
            int ret;
            copy.pNext = &exp;
            ret = orig_vkAllocateMemory(device, &copy, pAllocator, pMemory);
            if (ret == 0)
                return 0;
        }
    }
    return orig_vkAllocateMemory(device, pAllocateInfo, pAllocator, pMemory);
}


static void resolve_win32u_helpers(void)
{
    FILE *fp;
    char line[1024];
    uintptr_t base = 0;
    char fname[512];
    int fd;
    struct stat st;
    uint8_t *map;
    const Elf64_Ehdr *eh;
    const Elf64_Shdr *sh;
    uint16_t i;

    fname[0] = '\0';
    p_wine_server_handle_to_fd = (fn_wine_server_handle_to_fd_t)dlsym(NULL, "wine_server_handle_to_fd");
    fp = fopen("/proc/self/maps", "r");
    if (!fp) return;
    while (fgets(line, sizeof(line), fp)) {
        unsigned long lo = 0;
        char *p = strstr(line, "/win32u.so");
        if (p && sscanf(line, "%lx-", &lo) == 1) {
            char *slash = strchr(line, '/');
            if (slash) {
                size_t len = strcspn(slash, "\r\n");
                if (len < sizeof(fname)) {
                    memcpy(fname, slash, len);
                    fname[len] = '\0';
                    base = (uintptr_t)lo;
                    break;
                }
            }
        }
    }
    fclose(fp);
    if (!base || !fname[0]) return;

    fd = open(fname, O_RDONLY);
    if (fd < 0) return;
    if (fstat(fd, &st) == 0 && st.st_size > (off_t)sizeof(Elf64_Ehdr)) {
        map = mmap(NULL, (size_t)st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (map != MAP_FAILED) {
            eh = (const Elf64_Ehdr *)map;
            sh = (const Elf64_Shdr *)(map + eh->e_shoff);
            for (i = 0; i < eh->e_shnum; i++) {
                if (sh[i].sh_type == SHT_SYMTAB && sh[i].sh_link < eh->e_shnum) {
                    const Elf64_Sym *syms = (const Elf64_Sym *)(map + sh[i].sh_offset);
                    size_t nsyms = sh[i].sh_size / sizeof(Elf64_Sym);
                    const char *strtab = (const char *)(map + sh[sh[i].sh_link].sh_offset);
                    size_t j;
                    for (j = 0; j < nsyms; j++) {
                        const char *nm = strtab + syms[j].st_name;
                        if (strcmp(nm, "d3dkmt_open_resource") == 0)
                            p_d3dkmt_open_resource = (fn_d3dkmt_open_resource_t)(base + syms[j].st_value);
                        else if (strcmp(nm, "d3dkmt_object_get_fd") == 0)
                            p_d3dkmt_object_get_fd = (fn_d3dkmt_object_get_fd_t)(base + syms[j].st_value);
                        else if (strcmp(nm, "vulkan_funcs") == 0) {
                            void **vk_funcs = (void **)(base + syms[j].st_value);
                            if (vk_funcs[3] && vk_funcs[3] != (void *)hook_vkAllocateMemory) {
                                orig_vkAllocateMemory = (fn_vk_allocate_memory_t)vk_funcs[3];
                                vk_funcs[3] = (void *)hook_vkAllocateMemory;
                            }
                        }
                    }
                }
            }
            munmap(map, (size_t)st.st_size);
        }
    }
    close(fd);
    bridge_log("win32u helpers: open_resource=%p object_get_fd=%p handle_to_fd=%p\n",
               (void *)p_d3dkmt_open_resource, (void *)p_d3dkmt_object_get_fd, (void *)p_wine_server_handle_to_fd);
}

struct hip_ext_mem_handle_desc {
    int type;
    union {
        int fd;
        struct {
            void *handle;
            const void *name;
        } win32;
    } handle;
    unsigned long long size;
    unsigned int flags;
    unsigned int reserved[16];
};
static void * (*p___hipRegisterFatBinary)(const void * data);
static void (*p___hipUnregisterFatBinary)(void * modules);
static void (*p___hipRegisterFunction)(void * modules, const void * hostFunction, char * deviceFunction, const char * deviceName, unsigned int threadLimit, void * tid, void * bid, void * blockDim, void * gridDim, int * wSize);
static void (*p___hipRegisterVar)(void * modules, void * var, char * hostVar, const char * deviceName, int ext, size_t size, int constant, int global);
static int (*p_hipCreateTextureObject)(uint64_t * pTexObject, const void * pResDesc, const void * pTexDesc, const void * pResViewDesc);
static int (*p_hipDestroyTextureObject)(uint64_t textureObject);
static int (*p_hipDestroyExternalMemory)(void * extMem);
static int (*p_hipImportExternalMemory)(void ** extMem_out, const void * memHandleDesc);
static int (*p_hipExternalMemoryGetMappedBuffer)(void ** devPtr, void * extMem, const void * bufferDesc);
static int (*p_hipDeviceGetStreamPriorityRange)(int * leastPriority, int * greatestPriority);
static int (*p_hipDeviceSynchronize)(void);
static int (*p_hipDriverGetVersion)(int * driverVersion);
static int (*p_hipRuntimeGetVersion)(int * runtimeVersion);
static int (*p_hipEventCreate)(void ** event);
static int (*p_hipEventCreateWithFlags)(void ** event, unsigned int flags);
static int (*p_hipEventDestroy)(void * event);
static int (*p_hipEventElapsedTime)(float * ms, void * start, void * stop);
static int (*p_hipEventQuery)(void * event);
static int (*p_hipEventRecord)(void * event, void * stream);
static int (*p_hipEventSynchronize)(void * event);
static int (*p_hipFree)(void * ptr);
static int (*p_hipGetDevice)(int * deviceId);
static int (*p_hipGetDeviceCount)(int * count);
static int (*p_hipGetDevicePropertiesR0600)(void * prop, int device);
static const char * (*p_hipGetErrorString)(int hipError);
static int (*p_hipGetLastError)(void);
static int (*p_hipHostFree)(void * ptr);
static int (*p_hipHostGetDevicePointer)(void ** devPtr, void * hstPtr, unsigned int flags);
static int (*p_hipHostMalloc)(void ** ptr, size_t size, unsigned int flags);
static int (*p_hipLaunchKernel)(const void * function_address, struct hip_dim3 numBlocks, struct hip_dim3 dimBlocks, void ** args, size_t sharedMemBytes, void * stream);
static int (*p_hipMalloc)(void ** ptr, size_t size);
static int (*p_hipMallocPitch)(void ** ptr, size_t * pitch, size_t width, size_t height);
static int (*p_hipMemcpy)(void * dst, const void * src, size_t sizeBytes, int kind);
static int (*p_hipMemcpy2D)(void * dst, size_t dpitch, const void * src, size_t spitch, size_t width, size_t height, int kind);
static int (*p_hipMemcpyAsync)(void * dst, const void * src, size_t sizeBytes, int kind, void * stream);
static int (*p_hipMemcpyFromSymbol)(void * dst, const void * symbol, size_t sizeBytes, size_t offset, int kind);
static int (*p_hipMemcpyToSymbol)(const void * symbol, const void * src, size_t sizeBytes, size_t offset, int kind);
static int (*p_hipMemset)(void * dst, int value, size_t sizeBytes);
static int (*p_hipMemset2D)(void * dst, size_t pitch, int value, size_t width, size_t height);
static int (*p_hipMemset2DAsync)(void * dst, size_t pitch, int value, size_t width, size_t height, void * stream);
static int (*p_hipMemsetAsync)(void * dst, int value, size_t sizeBytes, void * stream);
static int (*p_hipMemsetD16Async)(void * dst, unsigned short value, size_t count, void * stream);
static int (*p_hipOccupancyMaxActiveBlocksPerMultiprocessor)(int * numBlocks, const void * f, int blockSize, size_t dynSharedMemPerBlk);
static int (*p_hipSetDevice)(int deviceId);
static int (*p_hipStreamCreate)(void ** stream);
static int (*p_hipStreamCreateWithFlags)(void ** stream, unsigned int flags);
static int (*p_hipStreamCreateWithPriority)(void ** stream, unsigned int flags, int priority);
static int (*p_hipStreamDestroy)(void * stream);
static int (*p_hipStreamSynchronize)(void * stream);

static NTSTATUS u_init(void *a)
{
    struct init_params *p = a;
    const char *rocm = "/home/dkp/.local/share/neuralhub/rocm";
    char path[512];

    snprintf(path, sizeof(path), "%s/usr/lib/libnuma.so.1", rocm);
    dlopen(path, RTLD_NOW | RTLD_GLOBAL);
    snprintf(path, sizeof(path), "%s/opt/rocm/lib/librocprofiler-register.so.0", rocm);
    dlopen(path, RTLD_NOW | RTLD_GLOBAL);
    snprintf(path, sizeof(path), "%s/opt/rocm/lib/libhsa-runtime64.so.1", rocm);
    dlopen(path, RTLD_NOW | RTLD_GLOBAL);
    snprintf(path, sizeof(path), "%s/opt/rocm/lib/libamd_comgr.so.3", rocm);
    dlopen(path, RTLD_NOW | RTLD_GLOBAL);
    snprintf(path, sizeof(path), "%s/opt/rocm/lib/libamdhip64.so", rocm);
    lib_hip = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
    if (!lib_hip)
        lib_hip = dlopen("libamdhip64.so", RTLD_NOW | RTLD_GLOBAL);
    if (!lib_hip) {
        bridge_log("dlopen libamdhip64.so failed: %s\n", dlerror());
        p->status = -1;
        return 0;
    }
    resolve_win32u_helpers();
    p___hipRegisterFatBinary = dlsym(lib_hip, "__hipRegisterFatBinary");
    p___hipUnregisterFatBinary = dlsym(lib_hip, "__hipUnregisterFatBinary");
    p___hipRegisterFunction = dlsym(lib_hip, "__hipRegisterFunction");
    p___hipRegisterVar = dlsym(lib_hip, "__hipRegisterVar");
    p_hipCreateTextureObject = dlsym(lib_hip, "hipCreateTextureObject");
    p_hipDestroyTextureObject = dlsym(lib_hip, "hipDestroyTextureObject");
    p_hipDestroyExternalMemory = dlsym(lib_hip, "hipDestroyExternalMemory");
    p_hipImportExternalMemory = dlsym(lib_hip, "hipImportExternalMemory");
    p_hipExternalMemoryGetMappedBuffer = dlsym(lib_hip, "hipExternalMemoryGetMappedBuffer");
    p_hipDeviceGetStreamPriorityRange = dlsym(lib_hip, "hipDeviceGetStreamPriorityRange");
    p_hipDeviceSynchronize = dlsym(lib_hip, "hipDeviceSynchronize");
    p_hipDriverGetVersion = dlsym(lib_hip, "hipDriverGetVersion");
    p_hipRuntimeGetVersion = dlsym(lib_hip, "hipRuntimeGetVersion");
    p_hipEventCreate = dlsym(lib_hip, "hipEventCreate");
    p_hipEventCreateWithFlags = dlsym(lib_hip, "hipEventCreateWithFlags");
    p_hipEventDestroy = dlsym(lib_hip, "hipEventDestroy");
    p_hipEventElapsedTime = dlsym(lib_hip, "hipEventElapsedTime");
    p_hipEventQuery = dlsym(lib_hip, "hipEventQuery");
    p_hipEventRecord = dlsym(lib_hip, "hipEventRecord");
    p_hipEventSynchronize = dlsym(lib_hip, "hipEventSynchronize");
    p_hipFree = dlsym(lib_hip, "hipFree");
    p_hipGetDevice = dlsym(lib_hip, "hipGetDevice");
    p_hipGetDeviceCount = dlsym(lib_hip, "hipGetDeviceCount");
    p_hipGetDevicePropertiesR0600 = dlsym(lib_hip, "hipGetDevicePropertiesR0600");
    p_hipGetErrorString = dlsym(lib_hip, "hipGetErrorString");
    p_hipGetLastError = dlsym(lib_hip, "hipGetLastError");
    p_hipHostFree = dlsym(lib_hip, "hipHostFree");
    p_hipHostGetDevicePointer = dlsym(lib_hip, "hipHostGetDevicePointer");
    p_hipHostMalloc = dlsym(lib_hip, "hipHostMalloc");
    p_hipLaunchKernel = dlsym(lib_hip, "hipLaunchKernel");
    p_hipMalloc = dlsym(lib_hip, "hipMalloc");
    p_hipMallocPitch = dlsym(lib_hip, "hipMallocPitch");
    p_hipMemcpy = dlsym(lib_hip, "hipMemcpy");
    p_hipMemcpy2D = dlsym(lib_hip, "hipMemcpy2D");
    p_hipMemcpyAsync = dlsym(lib_hip, "hipMemcpyAsync");
    p_hipMemcpyFromSymbol = dlsym(lib_hip, "hipMemcpyFromSymbol");
    p_hipMemcpyToSymbol = dlsym(lib_hip, "hipMemcpyToSymbol");
    p_hipMemset = dlsym(lib_hip, "hipMemset");
    p_hipMemset2D = dlsym(lib_hip, "hipMemset2D");
    p_hipMemset2DAsync = dlsym(lib_hip, "hipMemset2DAsync");
    p_hipMemsetAsync = dlsym(lib_hip, "hipMemsetAsync");
    p_hipMemsetD16Async = dlsym(lib_hip, "hipMemsetD16Async");
    p_hipOccupancyMaxActiveBlocksPerMultiprocessor = dlsym(lib_hip, "hipOccupancyMaxActiveBlocksPerMultiprocessor");
    p_hipSetDevice = dlsym(lib_hip, "hipSetDevice");
    p_hipStreamCreate = dlsym(lib_hip, "hipStreamCreate");
    p_hipStreamCreateWithFlags = dlsym(lib_hip, "hipStreamCreateWithFlags");
    p_hipStreamCreateWithPriority = dlsym(lib_hip, "hipStreamCreateWithPriority");
    p_hipStreamDestroy = dlsym(lib_hip, "hipStreamDestroy");
    p_hipStreamSynchronize = dlsym(lib_hip, "hipStreamSynchronize");
    bridge_log("amdhip64_7.so initialized: lib_hip=%p\n", lib_hip);
    p->status = 0;
    return 0;
}

static NTSTATUS u_fn_hipRegisterFatBinary(void *a)
{
    struct params_fn_hipRegisterFatBinary *p = a;
    p->ret = p___hipRegisterFatBinary(p->data);
    bridge_log("__hipRegisterFatBinary -> %p\n", p->ret);
    return 0;
}

static NTSTATUS u_fn_hipUnregisterFatBinary(void *a)
{
    struct params_fn_hipUnregisterFatBinary *p = a;
    p___hipUnregisterFatBinary(p->modules);
    return 0;
}

static NTSTATUS u_fn_hipRegisterFunction(void *a)
{
    struct params_fn_hipRegisterFunction *p = a;
    p___hipRegisterFunction(p->modules, p->hostFunction, p->deviceFunction, p->deviceName, p->threadLimit, p->tid, p->bid, p->blockDim, p->gridDim, p->wSize);
    return 0;
}

static NTSTATUS u_fn_hipRegisterVar(void *a)
{
    struct params_fn_hipRegisterVar *p = a;
    p___hipRegisterVar(p->modules, p->var, p->hostVar, p->deviceName, p->ext, p->size, p->constant, p->global);
    return 0;
}

static NTSTATUS u_fn_hipCreateTextureObject(void *a)
{
    struct params_fn_hipCreateTextureObject *p = a;
    p->ret = p_hipCreateTextureObject(p->pTexObject, p->pResDesc, p->pTexDesc, p->pResViewDesc);
    return 0;
}

static NTSTATUS u_fn_hipDestroyTextureObject(void *a)
{
    struct params_fn_hipDestroyTextureObject *p = a;
    p->ret = p_hipDestroyTextureObject(p->textureObject);
    return 0;
}

static NTSTATUS u_fn_hipDestroyExternalMemory(void *a)
{
    struct params_fn_hipDestroyExternalMemory *p = a;
    p->ret = p_hipDestroyExternalMemory(p->extMem);
    return 0;
}

static NTSTATUS u_fn_hipImportExternalMemory(void *a)
{
    struct params_fn_hipImportExternalMemory *p = a;
    const struct hip_ext_mem_handle_desc *in_desc = (const struct hip_ext_mem_handle_desc *)p->memHandleDesc;
    struct hip_ext_mem_handle_desc linux_desc;
    if (!p_d3dkmt_open_resource)
        resolve_win32u_helpers();
    if (in_desc && in_desc->type != 1) {
        void *nt_handle = in_desc->handle.win32.handle;
        int unix_fd = -1;
        unsigned int local_res = 0, runtime_desc = 0, sync_obj = 0;
        if (((uintptr_t)nt_handle & 0xf0000000U) == 0x70000000U) {
            unix_fd = (int)((uintptr_t)nt_handle & 0x0fffffffU);
        } else if (p_d3dkmt_open_resource && p_d3dkmt_object_get_fd && nt_handle) {
            local_res = p_d3dkmt_open_resource(0, nt_handle, &runtime_desc, &sync_obj);
            if (local_res)
                unix_fd = p_d3dkmt_object_get_fd(local_res);
        }
        if (unix_fd < 0 && p_wine_server_handle_to_fd && nt_handle) {
            unsigned int flags = 0;
            if (p_wine_server_handle_to_fd(nt_handle, 0x10000000U, &unix_fd, &flags) != 0)
                unix_fd = -1;
        }
        linux_desc = *in_desc;
        linux_desc.type = 1;
        linux_desc.handle.win32.handle = NULL;
        linux_desc.handle.win32.name = NULL;
        linux_desc.handle.fd = unix_fd;
        p->ret = p_hipImportExternalMemory(p->extMem_out, &linux_desc);
        bridge_log("hipImportExternalMemory(type=%d, nt_handle=%p, size=%llu, flags=0x%x) -> local=0x%x fd=%d ret=%d extMem=%p\n",
                   in_desc->type, nt_handle, in_desc->size, in_desc->flags, local_res, unix_fd, p->ret,
                   p->extMem_out ? *p->extMem_out : NULL);
        return 0;
    }
    p->ret = p_hipImportExternalMemory(p->extMem_out, p->memHandleDesc);
    bridge_log("hipImportExternalMemory(direct) -> %d\n", p->ret);
    return 0;
}

static NTSTATUS u_fn_hipExternalMemoryGetMappedBuffer(void *a)
{
    struct params_fn_hipExternalMemoryGetMappedBuffer *p = a;
    p->ret = p_hipExternalMemoryGetMappedBuffer(p->devPtr, p->extMem, p->bufferDesc);
    bridge_log("hipExternalMemoryGetMappedBuffer -> %d\n", p->ret);
    return 0;
}

static NTSTATUS u_fn_hipDeviceGetStreamPriorityRange(void *a)
{
    struct params_fn_hipDeviceGetStreamPriorityRange *p = a;
    p->ret = p_hipDeviceGetStreamPriorityRange(p->leastPriority, p->greatestPriority);
    return 0;
}

static NTSTATUS u_fn_hipDeviceSynchronize(void *a)
{
    struct params_fn_hipDeviceSynchronize *p = a;
    p->ret = p_hipDeviceSynchronize();
    return 0;
}

static NTSTATUS u_fn_hipDriverGetVersion(void *a)
{
    struct params_fn_hipDriverGetVersion *p = a;
    p->ret = p_hipDriverGetVersion(p->driverVersion);
    return 0;
}

static NTSTATUS u_fn_hipRuntimeGetVersion(void *a)
{
    struct params_fn_hipRuntimeGetVersion *p = a;
    p->ret = p_hipRuntimeGetVersion(p->runtimeVersion);
    return 0;
}

static NTSTATUS u_fn_hipEventCreate(void *a)
{
    struct params_fn_hipEventCreate *p = a;
    p->ret = p_hipEventCreate(p->event);
    return 0;
}

static NTSTATUS u_fn_hipEventCreateWithFlags(void *a)
{
    struct params_fn_hipEventCreateWithFlags *p = a;
    p->ret = p_hipEventCreateWithFlags(p->event, p->flags);
    return 0;
}

static NTSTATUS u_fn_hipEventDestroy(void *a)
{
    struct params_fn_hipEventDestroy *p = a;
    p->ret = p_hipEventDestroy(p->event);
    return 0;
}

static NTSTATUS u_fn_hipEventElapsedTime(void *a)
{
    struct params_fn_hipEventElapsedTime *p = a;
    p->ret = p_hipEventElapsedTime(p->ms, p->start, p->stop);
    return 0;
}

static NTSTATUS u_fn_hipEventQuery(void *a)
{
    struct params_fn_hipEventQuery *p = a;
    p->ret = p_hipEventQuery(p->event);
    return 0;
}

static NTSTATUS u_fn_hipEventRecord(void *a)
{
    struct params_fn_hipEventRecord *p = a;
    p->ret = p_hipEventRecord(p->event, p->stream);
    return 0;
}

static NTSTATUS u_fn_hipEventSynchronize(void *a)
{
    struct params_fn_hipEventSynchronize *p = a;
    p->ret = p_hipEventSynchronize(p->event);
    return 0;
}

static NTSTATUS u_fn_hipFree(void *a)
{
    struct params_fn_hipFree *p = a;
    p->ret = p_hipFree(p->ptr);
    return 0;
}

static NTSTATUS u_fn_hipGetDevice(void *a)
{
    struct params_fn_hipGetDevice *p = a;
    p->ret = p_hipGetDevice(p->deviceId);
    return 0;
}

static NTSTATUS u_fn_hipGetDeviceCount(void *a)
{
    struct params_fn_hipGetDeviceCount *p = a;
    p->ret = p_hipGetDeviceCount(p->count);
    bridge_log("hipGetDeviceCount -> %d\n", p->ret);
    return 0;
}

static NTSTATUS u_fn_hipGetDevicePropertiesR0600(void *a)
{
    struct params_fn_hipGetDevicePropertiesR0600 *p = a;
    p->ret = p_hipGetDevicePropertiesR0600(p->prop, p->device);
    bridge_log("hipGetDevicePropertiesR0600 -> %d\n", p->ret);
    return 0;
}

static NTSTATUS u_fn_hipGetErrorString(void *a)
{
    struct params_fn_hipGetErrorString *p = a;
    p->ret = p_hipGetErrorString(p->hipError);
    return 0;
}

static NTSTATUS u_fn_hipGetLastError(void *a)
{
    struct params_fn_hipGetLastError *p = a;
    p->ret = p_hipGetLastError();
    return 0;
}

static NTSTATUS u_fn_hipHostFree(void *a)
{
    struct params_fn_hipHostFree *p = a;
    p->ret = p_hipHostFree(p->ptr);
    return 0;
}

static NTSTATUS u_fn_hipHostGetDevicePointer(void *a)
{
    struct params_fn_hipHostGetDevicePointer *p = a;
    p->ret = p_hipHostGetDevicePointer(p->devPtr, p->hstPtr, p->flags);
    return 0;
}

static NTSTATUS u_fn_hipHostMalloc(void *a)
{
    struct params_fn_hipHostMalloc *p = a;
    p->ret = p_hipHostMalloc(p->ptr, p->size, p->flags);
    return 0;
}

static NTSTATUS u_fn_hipLaunchKernel(void *a)
{
    struct params_fn_hipLaunchKernel *p = a;
    p->ret = p_hipLaunchKernel(p->function_address, p->numBlocks, p->dimBlocks, p->args, p->sharedMemBytes, p->stream);
    return 0;
}

static NTSTATUS u_fn_hipMalloc(void *a)
{
    struct params_fn_hipMalloc *p = a;
    p->ret = p_hipMalloc(p->ptr, p->size);
    return 0;
}

static NTSTATUS u_fn_hipMallocPitch(void *a)
{
    struct params_fn_hipMallocPitch *p = a;
    p->ret = p_hipMallocPitch(p->ptr, p->pitch, p->width, p->height);
    return 0;
}

static NTSTATUS u_fn_hipMemcpy(void *a)
{
    struct params_fn_hipMemcpy *p = a;
    p->ret = p_hipMemcpy(p->dst, p->src, p->sizeBytes, p->kind);
    return 0;
}

static NTSTATUS u_fn_hipMemcpy2D(void *a)
{
    struct params_fn_hipMemcpy2D *p = a;
    p->ret = p_hipMemcpy2D(p->dst, p->dpitch, p->src, p->spitch, p->width, p->height, p->kind);
    return 0;
}

static NTSTATUS u_fn_hipMemcpyAsync(void *a)
{
    struct params_fn_hipMemcpyAsync *p = a;
    p->ret = p_hipMemcpyAsync(p->dst, p->src, p->sizeBytes, p->kind, p->stream);
    return 0;
}

static NTSTATUS u_fn_hipMemcpyFromSymbol(void *a)
{
    struct params_fn_hipMemcpyFromSymbol *p = a;
    p->ret = p_hipMemcpyFromSymbol(p->dst, p->symbol, p->sizeBytes, p->offset, p->kind);
    return 0;
}

static NTSTATUS u_fn_hipMemcpyToSymbol(void *a)
{
    struct params_fn_hipMemcpyToSymbol *p = a;
    p->ret = p_hipMemcpyToSymbol(p->symbol, p->src, p->sizeBytes, p->offset, p->kind);
    return 0;
}

static NTSTATUS u_fn_hipMemset(void *a)
{
    struct params_fn_hipMemset *p = a;
    p->ret = p_hipMemset(p->dst, p->value, p->sizeBytes);
    return 0;
}

static NTSTATUS u_fn_hipMemset2D(void *a)
{
    struct params_fn_hipMemset2D *p = a;
    p->ret = p_hipMemset2D(p->dst, p->pitch, p->value, p->width, p->height);
    return 0;
}

static NTSTATUS u_fn_hipMemset2DAsync(void *a)
{
    struct params_fn_hipMemset2DAsync *p = a;
    p->ret = p_hipMemset2DAsync(p->dst, p->pitch, p->value, p->width, p->height, p->stream);
    return 0;
}

static NTSTATUS u_fn_hipMemsetAsync(void *a)
{
    struct params_fn_hipMemsetAsync *p = a;
    p->ret = p_hipMemsetAsync(p->dst, p->value, p->sizeBytes, p->stream);
    return 0;
}

static NTSTATUS u_fn_hipMemsetD16Async(void *a)
{
    struct params_fn_hipMemsetD16Async *p = a;
    p->ret = p_hipMemsetD16Async(p->dst, p->value, p->count, p->stream);
    return 0;
}

static NTSTATUS u_fn_hipOccupancyMaxActiveBlocksPerMultiprocessor(void *a)
{
    struct params_fn_hipOccupancyMaxActiveBlocksPerMultiprocessor *p = a;
    p->ret = p_hipOccupancyMaxActiveBlocksPerMultiprocessor(p->numBlocks, p->f, p->blockSize, p->dynSharedMemPerBlk);
    return 0;
}

static NTSTATUS u_fn_hipSetDevice(void *a)
{
    struct params_fn_hipSetDevice *p = a;
    p->ret = p_hipSetDevice(p->deviceId);
    bridge_log("hipSetDevice -> %d\n", p->ret);
    return 0;
}

static NTSTATUS u_fn_hipStreamCreate(void *a)
{
    struct params_fn_hipStreamCreate *p = a;
    p->ret = p_hipStreamCreate(p->stream);
    return 0;
}

static NTSTATUS u_fn_hipStreamCreateWithFlags(void *a)
{
    struct params_fn_hipStreamCreateWithFlags *p = a;
    p->ret = p_hipStreamCreateWithFlags(p->stream, p->flags);
    return 0;
}

static NTSTATUS u_fn_hipStreamCreateWithPriority(void *a)
{
    struct params_fn_hipStreamCreateWithPriority *p = a;
    p->ret = p_hipStreamCreateWithPriority(p->stream, p->flags, p->priority);
    return 0;
}

static NTSTATUS u_fn_hipStreamDestroy(void *a)
{
    struct params_fn_hipStreamDestroy *p = a;
    p->ret = p_hipStreamDestroy(p->stream);
    return 0;
}

static NTSTATUS u_fn_hipStreamSynchronize(void *a)
{
    struct params_fn_hipStreamSynchronize *p = a;
    p->ret = p_hipStreamSynchronize(p->stream);
    return 0;
}

/* ntdll looks up this exact symbol in the .so (Wine unixlib ABI). */
const bridge_unix_fn_t bridge_funcs[] __asm__("__wine_unix_call_funcs") = {
    u_init,
    u_fn_hipRegisterFatBinary,
    u_fn_hipUnregisterFatBinary,
    u_fn_hipRegisterFunction,
    u_fn_hipRegisterVar,
    u_fn_hipCreateTextureObject,
    u_fn_hipDestroyTextureObject,
    u_fn_hipDestroyExternalMemory,
    u_fn_hipImportExternalMemory,
    u_fn_hipExternalMemoryGetMappedBuffer,
    u_fn_hipDeviceGetStreamPriorityRange,
    u_fn_hipDeviceSynchronize,
    u_fn_hipDriverGetVersion,
    u_fn_hipRuntimeGetVersion,
    u_fn_hipEventCreate,
    u_fn_hipEventCreateWithFlags,
    u_fn_hipEventDestroy,
    u_fn_hipEventElapsedTime,
    u_fn_hipEventQuery,
    u_fn_hipEventRecord,
    u_fn_hipEventSynchronize,
    u_fn_hipFree,
    u_fn_hipGetDevice,
    u_fn_hipGetDeviceCount,
    u_fn_hipGetDevicePropertiesR0600,
    u_fn_hipGetErrorString,
    u_fn_hipGetLastError,
    u_fn_hipHostFree,
    u_fn_hipHostGetDevicePointer,
    u_fn_hipHostMalloc,
    u_fn_hipLaunchKernel,
    u_fn_hipMalloc,
    u_fn_hipMallocPitch,
    u_fn_hipMemcpy,
    u_fn_hipMemcpy2D,
    u_fn_hipMemcpyAsync,
    u_fn_hipMemcpyFromSymbol,
    u_fn_hipMemcpyToSymbol,
    u_fn_hipMemset,
    u_fn_hipMemset2D,
    u_fn_hipMemset2DAsync,
    u_fn_hipMemsetAsync,
    u_fn_hipMemsetD16Async,
    u_fn_hipOccupancyMaxActiveBlocksPerMultiprocessor,
    u_fn_hipSetDevice,
    u_fn_hipStreamCreate,
    u_fn_hipStreamCreateWithFlags,
    u_fn_hipStreamCreateWithPriority,
    u_fn_hipStreamDestroy,
    u_fn_hipStreamSynchronize,
};
