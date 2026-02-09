# Copilot / AI Agent Instructions for this repo

Purpose
- Provide concise, actionable guidance so an AI coding agent can be immediately productive working on this microcontroller sample repository.

Big picture (what to know first)
- This repo contains MCU sample code and drivers for the Nuvoton M2U51 family: CMSIS core, device headers, a shared StdDriver, and multiple SampleCode apps.
- Key directories:
  - [Library/CMSIS](Library/CMSIS) — CPU/core headers and CMSIS sources.
  - [Library/StdDriver/inc](Library/StdDriver/inc) and [Library/StdDriver/src](Library/StdDriver/src) — hardware API, preferred place for peripheral changes.
  - [SampleCode](SampleCode) — example apps. Notable examples:
    - [SampleCode/StdDriver/GPIO_OutputInput/main.c](SampleCode/StdDriver/GPIO_OutputInput/main.c) — typical SysInit/UART patterns.
    - [SampleCode/ISP/ISP_UART/main.c](SampleCode/ISP/ISP_UART/main.c) — ISP tool and XOM boot flow (see `XOM_BASE_ADDR`).
    - [SampleCode/nu_com_isp_sboot/CMakeLists.txt](SampleCode/nu_com_isp_sboot/CMakeLists.txt) — small CMake-based sample.

Build / flash / run workflows
- Keil/UVision is used by many samples (look for `*.uvprojx` and `KEIL` folders, e.g. SampleCode/ISP/ISP_UART/KEIL). Use the provided Keil projects for easiest flash/debug on target hardware.
- A few samples include CMake (SampleCode/nu_com_isp_sboot) — build with CMake when available for command-line builds. See that sample's `CMakeLists.txt` for expectations.
- Projects reference Nu-Link / NuMicro toolchains implicitly — flashing and debug usually happen through Keil + Nu-Link. If adding CLI build steps, prefer adding a CMake target rather than modifying Keil project files.

Project-specific conventions & patterns
- Initialization functions are centralized in application `SYS_Init()` and peripheral-specific `*_Init()` functions (see `SYS_Init` and `UART0_Init` in the examples). Follow this pattern for new samples.
- Protected register access: many init sequences call `SYS_UnlockReg()` / `SYS_LockReg()` around clock or multi-function pin changes — do not remove these.
- Use the StdDriver APIs rather than direct register writes when a wrapper exists (`CLK_EnableModuleClock`, `GPIO_SetMode`, `UART_Open`, etc.). New drivers should be added under `Library/StdDriver/inc` and `Library/StdDriver/src`.
- Naming conventions: core-peripheral init functions and macros follow `PERIPH_Action` (e.g., `CLK_EnableModuleClock`, `GPIO_SetMode`). Sample code prints `SystemCoreClock` and uses `SystemCoreClockUpdate()`.

Integration points & cross-component notes
- Boot and ISP flows: `SampleCode/ISP/ISP_UART/main.c` shows an XOM-based verification and then remaps the vector table (`FMC_SetVectorAddr`) before reset. Be careful when changing boot logic or memory layout.
- Crypto/AES: `SampleCode/nu_com_isp_sboot` contains `aes.c`/`aes.h` and a CMake sample; any cryptographic changes may affect boot/verification behavior.
- Startup code and device headers are in `Library/CMSIS` and `Library/Device/Nuvoton/M2U51` — low-level changes should be coordinated with these headers.

What to do when editing code
- Keep changes small and local to a sample or driver. Run (or instruct the user how to run) the Keil project before proposing larger refactors.
- Preserve hardware init semantics: unlocking/locking protected registers, enabling peripheral clocks, and setting multi-function pin mapping.
- When adding an API to `Library/StdDriver`, add both the header in `inc/` and the implementation in `src/`, and follow the style of nearby drivers.

References (good starting points)
- [SampleCode/StdDriver/GPIO_OutputInput/main.c](SampleCode/StdDriver/GPIO_OutputInput/main.c) — shows typical app structure and StdDriver usage.
- [SampleCode/ISP/ISP_UART/main.c](SampleCode/ISP/ISP_UART/main.c) — ISP/XOM boot flow and UART command parsing.
- [SampleCode/nu_com_isp_sboot/CMakeLists.txt](SampleCode/nu_com_isp_sboot/CMakeLists.txt) — CMake example for command-line builds.
- [Library/StdDriver/inc](Library/StdDriver/inc) and [Library/StdDriver/src](Library/StdDriver/src) — canonical driver implementations.

If anything is unclear
- Ask for the preferred build target (Keil vs CMake) and which hardware (Nu-Link variant) you'll be using. I can then adapt suggested commands or test steps.

-- End
