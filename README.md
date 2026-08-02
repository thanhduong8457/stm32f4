# STM32F4 Projects

Firmware examples, build templates, reference material, and control projects
for STM32F4 microcontrollers.

The STM32F1 projects previously stored in this repository have moved to
[stm32f1](https://github.com/thanhduong8457/stm32f1).

## Repository Layout

| Path | Purpose |
| --- | --- |
| `STM32_F4/CmakeProject` | CMake-based STM32F4 demos and control projects. |
| `STM32_F4/MakeFileProject` | Makefile-based examples, templates, and drivers. |
| `STM32_F4/reference` | STM32F407 documentation and board schematics. |
| `_document_` | Additional STM32F4 manuals and Discovery-kit documents. |

## Toolchain

The projects generally require the GNU Arm Embedded toolchain, CMake or Make,
and an ST-Link-compatible programmer:

```bash
brew install --cask gcc-arm-embedded
brew install cmake make stlink
```

Build commands vary by project. Start in the selected project directory and
check its `CMakeLists.txt`, `Makefile`, or local README before building.

## Branch Organization

Use `main` as the stable integration branch. Develop an individual firmware
project on a focused branch named `feature/<project-name>` and merge it only
after the project builds and its documentation is updated.
