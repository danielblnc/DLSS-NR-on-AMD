# Troubleshooting and performance reporting

Start with the [requirements](../README.md#requirements) and the [official release notes](https://github.com/danielblnc/DLSS-NR-on-AMD/releases). Record the exact version you tested; "latest" changes over time.

## Check the installation

1. Confirm the game is running with DirectX 12 and FSR enabled. Record the FSR version and quality preset, including FSRAA if used. If another mod provides FSR, include that mod's name and version in your report.
2. Check that `dlssnr_on_amd_setup.exe` and your copy of `nvngx_dlssnr.dll` are in the game executable's folder, as described in the [installation instructions](../README.md#install). Record which game executable you installed beside, especially if the game also has a launcher.
3. Check that the NVIDIA DLL version matches the [requirements](../README.md#requirements). In Windows, right-click the DLL, choose **Properties**, and check the version under **Details**.
4. Start the game, load a playable scene, and press **End** to open the overlay. Record whether the overlay opens and whether enabling the effect changes the image.
5. Reproduce the problem and save `dlssnr_on_amd.log` before another test or update. If no log is created, say so and include any installer error text.

To update or remove the mod, close the game and run the installer again, choosing **U** or **R** respectively.

## Describe the symptom

| Symptom | Useful details to include |
| --- | --- |
| Installation fails or the overlay does not open | Installer output, game executable name, graphics API, whether a log was created, and other mods or overlays in use. |
| Overlay opens but FSR is not detected or the image does not change | FSR version and quality setting, native or mod-provided FSR, overlay mode and settings, and a log from a playable scene. |
| Crash, black screen, or artifacts | Steps to reproduce, error text, HDR and frame generation settings, and whether the problem follows a resolution change, menu transition, or load. Include screenshots for image defects if possible. |
| Low FPS or stuttering | Comparable measurements using the procedure below, the scene tested, and the log from each run. |

For every symptom, include the mod and game versions, exact GPU model and VRAM, Adrenalin driver version, and Windows version. If the system has multiple GPUs (including an integrated GPU), list them and identify the GPU running the game if known. Report the last working mod version if this is a regression.

## Compare performance

The following procedure makes reports easier to compare; it does not promise a particular frame rate or diagnose the bottleneck by itself.

1. Choose a repeatable scene, save location, camera position, or built-in benchmark. Let loading and shader compilation finish before recording. Use the same warm-up and measurement duration for each run; for example, warm up for 30 seconds and then measure for 60 seconds.
2. Record the output resolution, render resolution or scale if known, FSR version and quality preset, graphics preset, ray tracing settings, HDR state, and mod settings. Include CPU, system RAM, GPU and VRAM, driver, game version, and other mods.
3. For an initial comparison, turn off frame generation and dynamic resolution if the game allows it. Keep FSR enabled and all other settings identical between runs. Record VSync and any FPS cap, since a cap can hide differences. If the issue requires frame generation or dynamic resolution, report a separate comparison with that setting enabled.
4. Measure a baseline with DLSS-NR disabled, then repeat with DLSS-NR enabled in **inline** mode for gameplay. State how you disabled the mod (overlay toggle or installer removal). Record the overlay intensity settings used for the enabled run. The README describes **async** as a photo-mode option; report async measurements separately and identify the mode.
5. Repeat both conditions, ideally three times each, and report the measurement tool, duration, average FPS, and frame time in milliseconds if available. Include individual runs or their range so variability is visible. Report missing measurements as unavailable.
6. Save the log for each enabled run and note any visual differences or artifacts. If timing data is present in the log or overlay, copy its exact labels and values alongside the overall game measurements; these measure different parts of execution.

You can use this table in the performance field of the issue form:

| Run | Mod state / mode | Average FPS | Frame time (ms, specify average or percentile) | Log / notes |
| --- | --- | --- | --- | --- |
| 1 | Disabled (baseline) | | | |
| 1 | Enabled / inline | | | |
| 2 | Disabled (baseline) | | | |
| 2 | Enabled / inline | | | |
| 3 | Disabled (baseline) | | | |
| 3 | Enabled / inline | | | |

Do not mix different resolutions, FSR presets, scenes, frame generation settings, or measurement tools in the same comparison. If you change a setting, label the result as a separate test.

## Submit a report

Search the [existing issues](https://github.com/danielblnc/DLSS-NR-on-AMD/issues) for the same game and symptom. Add your environment and logs to a matching report, or [open a bug or performance report](https://github.com/danielblnc/DLSS-NR-on-AMD/issues/new?template=bug-report.yml).

Attach `dlssnr_on_amd.log` to the form's log field. Review it for personal information, such as your Windows username in file paths, before posting. If GitHub does not accept the `.log` extension, attach a `.txt` copy. If the log is unavailable, explain that in the same field; a failure before log creation is still worth reporting.
