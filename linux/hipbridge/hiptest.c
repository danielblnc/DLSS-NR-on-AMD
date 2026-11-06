#include <stdio.h>
#include <string.h>
#define COBJMACROS
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <d3d12.h>

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

struct hip_ext_mem_buffer_desc {
    unsigned long long offset;
    unsigned long long size;
    unsigned int flags;
    unsigned int reserved[16];
};

static const GUID iid_ID3D12Device = {0x189819f1, 0x1db6, 0x4b57, {0xbe, 0x54, 0x18, 0x21, 0x33, 0x9b, 0x85, 0xf7}};
static const GUID iid_ID3D12Resource = {0x696442be, 0xa72e, 0x4059, {0xbc, 0x79, 0x5b, 0x5c, 0x98, 0x04, 0x0f, 0xad}};

int main(void)
{
    GetDesktopWindow();
    HMODULE h = LoadLibraryA("amdhip64_7.dll");
    HMODULE hd3d = LoadLibraryA("d3d12.dll");
    printf("LoadLibrary amdhip: %p, d3d12: %p\n", h, hd3d);
    if (!h || !hd3d) return 1;

    int (WINAPI *count)(int *) = (void *)GetProcAddress(h, "hipGetDeviceCount");
    int (WINAPI *props)(void *, int) = (void *)GetProcAddress(h, "hipGetDevicePropertiesR0600");
    int (WINAPI *import_ext)(void **, const void *) = (void *)GetProcAddress(h, "hipImportExternalMemory");
    int (WINAPI *get_mapped)(void **, void *, const void *) = (void *)GetProcAddress(h, "hipExternalMemoryGetMappedBuffer");
    int (WINAPI *memset_fn)(void *, int, size_t) = (void *)GetProcAddress(h, "hipMemset");
    int (WINAPI *sync_fn)(void) = (void *)GetProcAddress(h, "hipDeviceSynchronize");
    HRESULT (WINAPI *create_dev)(IUnknown *, D3D_FEATURE_LEVEL, REFIID, void **) =
        (void *)GetProcAddress(hd3d, "D3D12CreateDevice");

    int n = -1, r = count(&n);
    char buf[0x5c0];
    memset(buf, 0, sizeof(buf));
    props(buf, 0);
    printf("HIP count=%d name='%s' arch='%s'\n", n, buf, buf + 0x488);

    ID3D12Device *dev = NULL;
    HRESULT hr = create_dev(NULL, D3D_FEATURE_LEVEL_11_0, &iid_ID3D12Device, (void **)&dev);
    printf("D3D12CreateDevice -> 0x%08lx dev=%p\n", (unsigned long)hr, dev);
    if (FAILED(hr) || !dev) return 1;

    D3D12_HEAP_PROPERTIES hp;
    memset(&hp, 0, sizeof(hp));
    hp.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC rd;
    memset(&rd, 0, sizeof(rd));
    rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    rd.Width = 65536;
    rd.Height = 1;
    rd.DepthOrArraySize = 1;
    rd.MipLevels = 1;
    rd.Format = DXGI_FORMAT_UNKNOWN;
    rd.SampleDesc.Count = 1;
    rd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    rd.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    ID3D12Resource *res = NULL;
    hr = ID3D12Device_CreateCommittedResource(dev, &hp, D3D12_HEAP_FLAG_SHARED, &rd,
                                              D3D12_RESOURCE_STATE_COMMON, NULL, &iid_ID3D12Resource, (void **)&res);
    printf("CreateCommittedResource -> 0x%08lx res=%p\n", (unsigned long)hr, res);

    HANDLE hShared = NULL;
    hr = ID3D12Device_CreateSharedHandle(dev, (ID3D12DeviceChild *)res, NULL, GENERIC_ALL, NULL, &hShared);
    printf("CreateSharedHandle -> 0x%08lx hShared=%p\n", (unsigned long)hr, hShared);

    struct hip_ext_mem_handle_desc desc;
    memset(&desc, 0, sizeof(desc));
    desc.type = 5;
    desc.handle.win32.handle = hShared;
    desc.size = 65536;
    desc.flags = 1;

    void *extMem = NULL;
    r = import_ext(&extMem, &desc);
    printf("hipImportExternalMemory -> %d extMem=%p\n", r, extMem);

    if (r == 0 && extMem) {
        struct hip_ext_mem_buffer_desc bdesc;
        void *devPtr = NULL;
        memset(&bdesc, 0, sizeof(bdesc));
        bdesc.size = 65536;
        r = get_mapped(&devPtr, extMem, &bdesc);
        printf("hipExternalMemoryGetMappedBuffer -> %d devPtr=%p\n", r, devPtr);
        if (r == 0 && devPtr) {
            r = memset_fn(devPtr, 0x5a, 65536);
            printf("hipMemset(shared D3D12 buffer) -> %d, sync -> %d\n", r, sync_fn());
        }
    }
    return 0;
}
