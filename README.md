# DLSS-NR on AMD

Run **DLSS 5 Neural Rendering** on **AMD Radeon RX 9000 (RDNA4)** cards, in any DirectX 12 game that uses FSR.

## Download

Get `dlssnr_on_amd_setup.exe` from the [latest release](../../releases/latest).

## Install

1. Place `dlssnr_on_amd_setup.exe` in the game folder, for Cyberpunk 2077 that is `bin\x64`.
2. Get a copy of `nvngx_dlssnr.dll` (DLSS 5 Neural Rendering, build 310.8.0.0, you can get it from games that support DLSS 5) and place it in the game folder.
3. Start the game and **enable FSR** (any FSR 3 / FSR 4 works, any quality / FSRAA). Press **End** to open the overlay.

Run `dlssnr_on_amd_setup.exe` again to **update** (U) or **remove** (R).

## What to expect

Output quality is very similar to what the official DLSS gets, though there are still some effects that may not be working 100%.

Performance is still WIP: roughly 28 fps at 1080p on an RX 9070 XT. I'm improving performance every day, goal is to get it to run as fast as a 5070Ti. Check back for new releases.

## In-game overlay

End key to open. Up/Down/Left/Right to adjust, Enter toggles.
Options: Mode (inline / async, for photo mode only), Tone intensity, Structure intensity, Skin structure. Model selection is planned.

## Requirements

- Windows 11, DirectX 12 game with FSR. Vulkan is planned for a later release. Tested on Cyberpunk and GTA V Enhanced.
- An AMD Radeon RX 9000 series GPU (tested on a 9070 XT). RX 7000 series cards are untested, reports are welcome. Older GPUs are not supported.
- A recent Adrenalin driver
- Your own copy of `nvngx_dlssnr.dll` version 310.8.0.0
- Games with anti-cheat will block the DLL; use them only with anti-cheat off.

## How can I help?

Two ways:
- Donate. Tokens are expensive, and GPUs to test are even more expensive. Donate here: [ko-fi.com/danielblnc](https://ko-fi.com/danielblnc)
- Test and report. Try on your 7000 GPU or other games, open an issue and attach `dlssnr_on_amd.log` if you encounter any problems.

## Legal

This project is not affiliated with or endorsed by NVIDIA or AMD. DLSS is a trademark of NVIDIA Corporation. The software here contains no NVIDIA code or data; it requires the user's own legitimately obtained copy of the DLSS 5 DLL. Provided as-is, without warranty; use at your own risk.
