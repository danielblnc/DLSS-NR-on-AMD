# DLSS-NR on AMD

Run **DLSS 5 Neural Rendering** on **AMD Radeon RX 9000 (RDNA4), 7000 (RDNA3) and 6000 (RDNA2)** cards, in any DirectX 12 game that uses FSR, DLSS or XeSS.

**NEW IN 0.8.0**: Every main DLSS feature is now fully ported. DLSS Super Resolution, DLSS Ray Reconstruction, DLSS Frame Generation and of course, DLSS Neural Rendering now work on every RDNA4 and RDNA3 GPU. RDNA2 is in testing, supports NR for now, and will receive most of the missing features in the next big release.
FSR Super Resolution is also included, and FSR Frame Generation and Ray Regeneration will be added in an upcoming build.

### Join the [Discord](https://discord.gg/5gCwc6mskc) for support, early access to new mod builds and testing.

## Download

Get `NeuralHub_vX.X.X_setup.exe` from the [latest release](../../releases/latest).

## Install

1. Run `NeuralHub_vX.X.X_setup.exe` and follow the installation steps.
2. Get a copy of `nvngx_dlssnr.dll` (DLSS 5 Neural Rendering, build 310.8.0.0 or later, you can get it from games that support DLSS 5) and import it into NeuralHub.
3. Optionally import `nvngx_dlssg.dll`, `nvngx_dlss.dll`, and `nvngx_dlssd.dll` to use the DLSS-FG, DLSS-SR, and DLSS-RR features. FSR is bundled with the manager and ready to go.
4. Open NeuralHub and let it scan for games.
5. Install the mod loader in every game you'd like and configure the DLSS or FSR features you want.
6. Start the game and **enable DLSS, FSR or XeSS** (any quality / native antialiasing). For now, DLSS Frame Generation requires spoofing and DLSS-FG inputs. Press **End** to open the in-game overlay.

## What to expect

Output quality is very similar to what DLSS produces on the original hardware, though there are still some effects that may not be working 100%.

Performance is now playable: 90-100 FPS at 1080p on an RX 9070 XT, using the full HT model. I'm improving performance every day, goal is to get it to run as fast as a 5070Ti. Check back for new releases.

## In-game overlay

End key to open. Up/Down/Left/Right to adjust, Enter toggles, PgUp/Down for previous/next tab.

## Requirements

- Windows 11, DirectX 12 game with DLSS, FSR or XeSS. Vulkan and Linux support are planned for an upcoming release.
- An AMD Radeon RX 9000, 7000, or 6000 series GPU.
- Installed Adrenalin driver 26.1.1 or later
- Your own copy of the mentioned DLLs
- Games with anti-cheat will block the DLL; use them only with anti-cheat off.

## Linux (experimental)

`linux/neuralhub-wine.sh` runs the manager itself on Linux through Wine + WebView2 and points it at your native Steam library:

```sh
linux/neuralhub-wine.sh ~/Downloads/NeuralHub_v0.8.0_setup.exe
```

To install the mod runtime, genuine `cabinet.dll`, and the HIP bridge into any Proton game:

```sh
# 1. Build the HIP bridge
linux/hipbridge/build.sh

# 2. Deploy to game binaries folder and Proton prefix
linux/install-mod.sh /path/to/game/Binaries/Win64 /path/to/compatdata/<appid>/pfx

# 3. Launch the game through Steam with Launch Options:
WINEDLLPATH="/path/to/DLSS-NR-on-AMD/linux/hipbridge/out" WINEDLLOVERRIDES="version=n,b;cabinet=n,b;amdhip64_7=n,b" %command%
```

In-game under Proton (`WINEDLLOVERRIDES="version=n,b"`), the stub, host hooks, and bundled FSR 4.1.1 upscaler load, and replacing Proton's builtin `cabinet.dll` with a native Windows copy (`cabinet=n,b`) gets past the initial `CreateDecompressor(LZMS)` failure to unpack the embedded `gfx*` code objects:

1. **D3D12 compute backend:** `neuralhub_host.dll` carries no DXIL shaders — it cross-compiles its AMDGPU HSA code objects at runtime through AMD's Windows driver extension (`amdxc64.dll` → `IAmdExtD3DDevice7`), which vkd3d-proton does not implement (`0x80004002` / `E_NOINTERFACE`). Wine's `amdxc64.dll` is a stub whose `AmdExtD3DDevice8_QueryInterface` (`0x180002470`) returns `0x80004002` unconditionally, and the release build compiles the Vulkan compute backend out (`DLSSNR_BUILD_PROFILE=release`).
2. **HIP fallback backend (`linux/hipbridge/`):** `linux/hipbridge/build.sh` builds a Wine builtin `amdhip64_7.dll` (PE) + `amdhip64_7.so` (unixlib) that forwards all 49 HIP symbols used by `neuralhub_host.dll` to native Linux ROCm (`libamdhip64.so`), hooks `win32u`'s `vkAllocateMemory` so vkd3d-proton shared D3D12 buffers are exported as DMA-BUFs, and unwraps Win32 shared `HANDLE`s via `d3dkmt_open_resource` + `d3dkmt_object_get_fd` in `hipImportExternalMemory`. With this bridge (`WINEDLLPATH=.../linux/hipbridge/out`, `WINEDLLOVERRIDES="version=n,b;cabinet=n,b;amdhip64_7=n,b"`), `neuralhub_host.dll` loads the HIP runtime in-game under Proton, registers all 4 fat binaries (360 kernels, 11 device variables), passes its GPU queue alignment probe on `gfx1200`, and enables the HIP compute backend (`compute: the HIP runtime serves every addon in this process`). Note that DLSS-FG in v0.8.0 requires the D3D12 compute backend, while DLSS-NR runs on the HIP backend once `nvngx_dlssnr.dll` (310.8.0.0+) weights are imported.

Vulkan and native Linux support are planned (see Requirements).

## How can I help?

Two ways:
- Donate. Tokens are expensive, and hardware to test my mods on are even more expensive. Donate here: [ko-fi.com/danielblnc](https://ko-fi.com/danielblnc)
- Test and report. Try on your games, open an issue and attach `dlssnr_on_amd.log` if you encounter any problems. Join the discord for faster communication.

## FAQ

- **Is this a compatibility layer?** No. This is a ground-up, full reimplementation of every DLSS neural network runtime for AMD GPU architectures (HIP kernels, memory layouts, game integration), written from scratch. No NVIDIA code is included or translated at runtime.

## Legal

This project is not affiliated with or endorsed by NVIDIA or AMD. DLSS is a trademark of NVIDIA Corporation, and all credit of the DLSS program goes to them. The software here contains no NVIDIA code or data; it requires the user's own legitimately obtained copy of the DLSS DLLs. Provided as-is, without warranty; use at your own risk.
