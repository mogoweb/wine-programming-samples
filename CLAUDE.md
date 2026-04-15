# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

This repository contains Windows programming samples used to study Wine/Wine Wayland compatibility. The samples demonstrate various Windows APIs including:
- Layered windows (per-pixel alpha transparency, shadow borders)
- Basic window creation and management
- SetParent cross-process window attachment
- Input Message Editor (IME) handling
- Vulkan integration
- CEF (Chromium Embedded Framework) integration
- Time/timer examples

## Building

### Cross-compilation (Linux)

Most samples use MinGW-w64 for cross-compilation:

```bash
cd <sample-directory>
make
```

Common make target patterns:
- `make` - Build all targets (typically .exe files)
- `make clean` - Clean built artifacts

Default compiler per sample:
- `i686-w64-mingw32-gcc` or `i686-w64-mingw32-g++` (32-bit Windows)
- `x86_64-w64-mingw32-g++` (64-bit Windows - e.g., hello-cef)

For native Windows compilation (MSYS2/MinGW):
```bash
make CC=gcc
```

### Project Notes

- **hello-vulkan**: Uses native `gcc` with `-lvulkan` (Linux native, not Windows)
- **hello-edit**: Links against `-limm32 -lgdi32 -static` for IME support
- **window/setparent**: Has a `help` target showing Wine Wayland run instructions
- **Windows graphics programming**: Located in `windows-graphics-programming/CH_01/`

## Running with Wine

Samples are designed to run under Wine/Wine Wayland:

```bash
# Basic Wine execution
wine sample.exe

# Wine Wayland (from window/setparent help)
DISPLAY= WAYLAND_DISPLAY=wayland-1 WINEFSYNC=1 wine sample.exe
```

## Architecture

Each sample is an independent directory with its own Makefile. No common build system or shared libraries between samples.

### Sample Structure

```
<sample-name>/
├── Makefile         # Build configuration
├── main.cpp/c       # Source code
├── *.vcxproj       # Visual Studio project (for Windows dev)
└── *.sln           # Visual Studio solution (for Windows dev)
```

### Key Samples

- **layered-window/**: Three demos showing different layered window API usage patterns
- **window/setparent/**: Cross-process window parent/child relationship demo with Wine Wayland testing
- **hello-win/**: Basic Win32 window example with geometry output
- **hello-edit/**: IME (Input Method Editor) message handling demonstration
- **hello-cef/**: Minimal CEF integration with embedded browser SDK in `sdk/`

## Commit Message Rules

Format:
<type>: <short summary>

<body>

Rules:
- Do NOT include Co-authored-by
- Do NOT mention AI assistance
- Keep summary under 72 chars
- Use imperative mood