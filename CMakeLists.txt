# ============================================================================
#  OpenDeFeedback — top-level CMake
#
#  Realtime live-audio cleanup plugin (de-feedback / de-noise / de-reverb)
#  built on iPlug2 + IGraphics (NanoVG backend).
#
#  Quick start (Windows / Visual Studio):
#     git submodule update --init --recursive
#     # one-time: fetch the VST3 SDK (see README) for the VST3 target
#     cmake -B build -G "Visual Studio 17 2022" -A x64
#     cmake --build build --config Release --target OpenDeFeedback-vst3
#
#  The standalone APP target builds without any external SDK and is handy for
#  quickly compiling / auditioning the DSP:
#     cmake --build build --config Release --target OpenDeFeedback-app
# ============================================================================
cmake_minimum_required(VERSION 3.14)

project(OpenDeFeedback VERSION 0.1.0 LANGUAGES C CXX)

# --- Locate the iPlug2 submodule -------------------------------------------
if(NOT DEFINED IPLUG2_DIR)
  set(IPLUG2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/iPlug2" CACHE PATH "iPlug2 root directory")
endif()

if(NOT EXISTS "${IPLUG2_DIR}/iPlug2.cmake")
  message(FATAL_ERROR
    "iPlug2 not found at ${IPLUG2_DIR}.\n"
    "Run:  git submodule update --init --recursive")
endif()

# iPlug2 pins the C++ standard (C++17). We stay compatible with that.
include(${IPLUG2_DIR}/iPlug2.cmake)

find_package(iPlug2 REQUIRED)

add_subdirectory(src/plugin)
