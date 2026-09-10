# Linux and Proton investigation notes

These notes describe an incomplete test of the DLSS 5 Neural Rendering
component on Linux. They are intended for the upstream author, not as install
instructions. No Linux release is available from this repository.

## Test status

The test used this environment:

- Resident Evil Requiem on an RX 9070 XT (`gfx1201`)
- DLSS-NR on AMD v0.2.17, build `976a3fa0`
- a local diagnostic copy of the integration DLL with its two process-wide
  exception-handler registrations suppressed
- Proton `experimental-11.0-20260903c-x86_64`
- a local vkd3d-proton build based on commit
  `634d341a5a312a3d75bb5cc6ac522534f1e8b454`, with shared-resource export
  and graphics/HIP ordering changes
- ROCm SDK prerelease `7.14.0a20260612`, HIP runtime `71460850`
- async mode at 5120x1440

The integration found FSR 4.0.3.604 and FSR loader 2.1.0.604. It initialized
native Linux HIP, selected the RX 9070 XT, imported shared resources, and
completed at least 400 neural jobs. The log recorded job 100 at 123 ms. Later
samples in the same run were slower.

The rendered image was mostly black. This test confirms that the modules loaded
and the neural kernels ran in this configuration. It does not establish correct
output or support for stock v0.2.17.

The recorded artifacts had these SHA-256 hashes:

| Component | SHA-256 |
| --- | --- |
| Upstream v0.2.17 integration DLL before diagnostics | `76404a80a05cbdd75b9b4c9f193d8637f684921a9b5cde365fd4512b4b362ef2` |
| Diagnostic integration DLL used for the run | `08dce856b70ac3e6a818aa30338beb2fe3d0ccdfec3a66bfff6ba2b87395f0bc` |
| PE HIP bridge | `d3e3534e0a5755bf7744ce61f89984075d9e486c829fa5ffe62d70f8f15fb6b6` |
| ELF HIP bridge | `d2203d0ee645e64a3d396b85767eebe476fb68cc56eb5f9f1d6e92df8c9db7e0` |
| vkd3d-proton `d3d12.dll` | `1fb057d0c190f805aa02df863755829ebb57e6a44604ec205faca986dd5234eb` |
| vkd3d-proton `d3d12core.dll` | `8d7a3bc66f584565ff6eddde95b78682576ebef9954e9b766fe1d3a9dcb4dfaf` |
| Diagnostic injector | `c5529d2ff0c49a20f6ce17d110b634ceabbb4a3b459b9757a54730128a3e149b` |

These hashes identify the exact local files. The components and raw logs are not
part of this pull request, so the result cannot be reproduced from this
repository alone.

## Loader observations

Requiem displayed the same game error when the test loaded either the
integration DLL or an 8 KB no-op DLL from the game directory. A no-op DLL loaded
from the Proton prefix's `C:\windows\system32` directory passed that boundary.

The neural jobs ran with this layout:

```text
Steam
  → Proton
  → diagnostic pre-entry injection
  → integration DLL outside the game directory
  → PE HIP bridge in the prefix system32 directory
  → preloaded ELF bridge
  → native Linux HIP
```

The injector started the game suspended, allowed the Windows loader to finish,
loaded the integration DLL, and then resumed the game's entry point. It changed
process memory and used a remote thread. No game file was patched on disk.

This result supports one narrow conclusion. DLL placement affects Requiem before
the integration reaches HIP. The tested `version.dll` proxy path failed. The
diagnostic pre-entry path succeeded after placement and handler behavior also
changed, so the comparison does not isolate load timing.

The injector is research code, not a proposed shipping component. Remote-thread
injection can trigger security software and must not be used with anti-cheat.

## Source changes to test

The public repository does not include the integration-layer source. The items
below are hypotheses for Daniel to test in that source. They are not binary
patch instructions.

### Test a later supported load point

The pre-entry injector avoided the failure seen through the proxy path, but it
also changed module placement and handler behavior. Add a supported delayed-load
option, then compare it with proxy loading while holding the DLL, module paths,
configuration, prefix, and handler setting constant. This test can establish
whether timing matters without making process injection part of the product.

### Resolve the HIP bridge outside the game directory

Allow an absolute bridge path in the configuration. Otherwise, restrict lookup
to the Proton prefix's system directory.

```text
function load_hip_bridge(config):
    if config has an absolute bridge path:
        flags = LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR
              | LOAD_LIBRARY_SEARCH_SYSTEM32
        return LoadLibraryExW(config.bridge_path, NULL, flags)

    return LoadLibraryExW("amdhip64_7.dll", NULL,
                          LOAD_LIBRARY_SEARCH_SYSTEM32)
```

Log the resolved path and file version. Stop before installing graphics hooks
if the bridge fails to load.

Test this with the same no-op DLL in the game directory and prefix system
directory. Then repeat with the real PE bridge. The resolved path in the log
must match the loaded module path.

### Make exception diagnostics optional

The successful run suppressed two process-wide handler registrations, but the
test did not isolate that change after fixing DLL placement. Add a setting that
disables those handlers without changing initialization or hooks. Compare both
settings in otherwise identical runs.

If the handlers are needed during setup, remove them when setup finishes. A
long-lived handler should log integration faults and return `CONTINUE_SEARCH`
for exceptions outside the integration modules.

### Treat shader fallback as an error

The v0.2.17 log reported this compile failure:

```text
Method 'Load' is not defined on type 'RWByteAddressBuffer'.
```

Use a read operation supported by the compiler and shader model selected under
Proton. A non-mutating atomic read may work if it matches the integration
layer's documented flag layout. Compile the shader with both the Windows and
Proton compiler paths in CI. Either path failing should fail the test.

### Add render-target residual composition

The tested output was `DXGI_FORMAT_R9G9B9E5_SHAREDEXP` (67) without the UAV flag.
The integration logged that residual application was unavailable and replaced
the frame. Frame replacement was active during the incorrect-output run and is
one candidate cause.

When the output supports a typed UAV, keep the existing compute compositor. For
an output that supports RTV but not UAV, test a graphics path:

```text
copy the original game output to a shader-readable scratch texture
transition the scratch texture and neural residual for pixel-shader reads
transition the game output to a render target
draw a full-screen triangle that applies the residual and color conversion
restore the resource state expected by the game
```

Cache the scratch texture, descriptors, and pipeline by device, size, format,
and color space. Recreate them only after prior GPU work completes. If neither
UAV nor RTV composition is valid, skip neural output for that frame and log one
rate-limited error. Do not replace the original frame.

Validate SDR, HDR, resize, dynamic resolution, context recreation, menus, and
motion before treating this path as correct.

### Preserve graphics and HIP ordering

Shared memory still needs explicit ownership and completion ordering:

```text
submit graphics work that produces the inputs
signal producer completion and wait before HIP reads
run the neural kernels
signal HIP completion and wait before graphics consumes the result
```

Use a synchronization method supported by both APIs. If Proton cannot expose a
compatible external semaphore, split producer and consumer submissions and use
a bounded CPU handoff. On timeout, disable the integration or remove the D3D12
device. Never allow concurrent writes to the shared resource.

## Performance hypothesis

A Cyberpunk diagnostic run logged a neural job near 161 ms with CPU staging. A
different Requiem run logged a single 123 ms job on the zero-copy path. The
games, scenes, and configurations differ, so these samples cannot be compared.

For a future same-scene test that starts near 161 ms and targets 125 ms, the
working hypothesis is to remove copies and repeated setup from the steady-state
path:

- allocate shareable, dedicated D3D12 resources once;
- import each dma-buf into HIP once and cache the mapping;
- reuse descriptors, pipelines, events, and streams;
- avoid full-device waits between producer, HIP, and consumer work;
- use async mode only after resource ownership and lifetime tests pass.

Measure repeated runs with frame-time percentiles. A single neural-job timing is
not a frame-rate result.

## Validation before release

Use a clean prefix and unmodified game installation. Record hashes and resolved
module paths before launch.

| Check | Pass condition |
| --- | --- |
| PE HIP probe | Loads the PE bridge and calls runtime version and device count |
| Native HIP probe | Selects the expected GPU and passes a small copy round trip |
| Shared-resource identity | Graphics writes a known pattern, HIP transforms it, and graphics verifies the result |
| Ordering stress | Repeats the shared-resource test without stale reads, concurrent access, or timeout |
| FSR hook | Logs exact loader and API versions |
| Neural dispatch | Completes 200 sequential jobs without a timeout |
| Shader compilation | Has no compile failure or fallback |
| Output path | Uses residual composition and never replaces the frame |
| Visual check | Passes SDR, HDR, resize, menu, and motion captures |
| Performance | Reports repeated disabled/enabled frame-time percentiles |
| Failure handling | Missing bridge, timeout, and device loss leave the prefix usable |

First repeat the Requiem test on a second machine. Then run the same clean setup
on a non-RE Engine DirectX 12 game with FSR and no anti-cheat. Do not call the
Linux path supported until both produce correct images.

## Credit and provenance

Daniel Blanco created DLSS-NR on AMD, including its Windows integration and
neural kernels. This document contains no Daniel, NVIDIA, AMD, or game binaries.
OpenAI Codex drafted these notes, and the submitter checked them against the
captured test logs.
