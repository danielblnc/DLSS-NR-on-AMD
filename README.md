# DLSS-NR on AMD

[English](README.md) | [简体中文](README_zh-CN.md)

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

## How can I help?

Two ways:
- Donate. Tokens are expensive, and hardware to test my mods on are even more expensive. Donate here: [ko-fi.com/danielblnc](https://ko-fi.com/danielblnc)
- Test and report. Try on your games, open an issue and attach `dlssnr_on_amd.log` if you encounter any problems. Join the discord for faster communication.

## FAQ

- **Is this a compatibility layer?** No. This is a ground-up, full reimplementation of every DLSS neural network runtime for AMD GPU architectures (HIP kernels, memory layouts, game integration), written from scratch. No NVIDIA code is included or translated at runtime.

## Legal

This project is not affiliated with or endorsed by NVIDIA or AMD. DLSS is a trademark of NVIDIA Corporation, and all credit of the DLSS program goes to them. The software here contains no NVIDIA code or data; it requires the user's own legitimately obtained copy of the DLSS DLLs. Provided as-is, without warranty; use at your own risk.
