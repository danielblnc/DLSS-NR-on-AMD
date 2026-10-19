# DLSS-NR on AMD

[English](README.md) | 简体中文

在 **AMD Radeon RX 9000（RDNA4）、7000（RDNA3）与 6000（RDNA2）** 显卡上运行 **DLSS 5 Neural Rendering**，适用于任何使用 FSR、DLSS 或 XeSS 的 DirectX 12 游戏。

**0.8.0 新增**：所有主要 DLSS 功能现已完整移植。DLSS Super Resolution、DLSS Ray Reconstruction、DLSS Frame Generation，以及 DLSS Neural Rendering，均可在所有 RDNA4 与 RDNA3 GPU 上运行。RDNA2 正在测试中，目前已支持 NR，其余大部分功能将在下一个大版本中提供。
FSR Super Resolution 也已包含在内，FSR Frame Generation 与 Ray Regeneration 将在后续版本中加入。

### 加入 [Discord](https://discord.gg/5gCwc6mskc) 获取支持、抢先体验新 mod 版本并参与测试。

## 下载

从[最新发布页](../../releases/latest)获取 `NeuralHub_vX.X.X_setup.exe`。

## 安装

1. 运行 `NeuralHub_vX.X.X_setup.exe`，按照安装步骤完成安装。
2. 准备一份 `nvngx_dlssnr.dll`（DLSS 5 Neural Rendering，版本 310.8.0.0 或更高，可从支持 DLSS 5 的游戏中提取），并导入 NeuralHub。
3. （可选）导入 `nvngx_dlssg.dll`、`nvngx_dlss.dll` 与 `nvngx_dlssd.dll`，以使用 DLSS-FG、DLSS-SR 与 DLSS-RR 功能。FSR 已随管理器捆绑，开箱即用。
4. 打开 NeuralHub，等待其扫描游戏。
5. 在想要使用的游戏中安装 mod 加载器，并配置所需的 DLSS 或 FSR 功能。
6. 启动游戏并**启用 DLSS、FSR 或 XeSS**（任意画质档位 / 原生抗锯齿）。目前，DLSS Frame Generation 需要伪装（spoofing）与 DLSS-FG 输入支持。按 **End** 键打开游戏内 overlay。

## 效果预期

输出画质与 DLSS 在原生硬件上的表现非常接近，但仍有部分效果可能无法 100% 正常工作。

性能现已达到可玩水平：在 RX 9070 XT 上使用完整 HT 模型，1080p 下可达 90-100 FPS。性能每天都在优化，目标是媲美 5070Ti。请关注后续版本。

## 游戏内 overlay

按 End 键打开。上/下/左/右方向键调节，Enter 切换开关，PgUp/PgDown 切换上一个/下一个标签页。

## 系统要求

- Windows 11；使用 DLSS、FSR 或 XeSS 的 DirectX 12 游戏。Vulkan 与 Linux 支持计划在后续版本提供。
- AMD Radeon RX 9000、7000 或 6000 系列 GPU。
- Adrenalin 驱动程序 26.1.1 或更高版本
- 自行获取上述 DLL 文件
- 带反作弊的游戏会阻止该 DLL，请仅在关闭反作弊的情况下使用

## 如何提供帮助？

两种方式：
- 捐赠。Token 很贵，而用于测试 mod 的硬件更贵。在此捐赠：[ko-fi.com/danielblnc](https://ko-fi.com/danielblnc)
- 测试与反馈。在你的游戏中试用；遇到问题时，请提交 issue 并附上 `dlssnr_on_amd.log`。加入 Discord 可更快交流。

## FAQ

- **这是一个兼容层吗？** 不是。这是针对 AMD GPU 架构，对每一个 DLSS 神经网络运行时的彻底重写（HIP kernel、内存布局、游戏集成），完全从零编写。运行时不包含、也不转译任何 NVIDIA 代码。

## 法律声明

本项目与 NVIDIA 或 AMD 无任何隶属关系，也未获得其认可。DLSS 是 NVIDIA Corporation 的商标，DLSS 计划的全部荣誉归于 NVIDIA。本软件不包含任何 NVIDIA 代码与数据，需由用户自行合法获取 DLSS DLL 的副本。按“原样”提供，不作任何担保；使用风险自负。
