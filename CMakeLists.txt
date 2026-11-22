cmake_minimum_required(VERSION 3.24)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
project(ValorantSensFinder LANGUAGES C CXX)

if(NOT WIN32)
    message(FATAL_ERROR "ValorantSensFinder requires Windows for Win32 Raw Input.")
endif()

include(FetchContent)

set(BUILD_EXAMPLES OFF CACHE BOOL "Build raylib examples")
set(BUILD_SHARED_LIBS OFF)
set(USE_EXTERNAL_GLFW OFF)
set(USE_MSVC_RUNTIME_LIBRARY_DLL OFF CACHE BOOL "Use the GLFW MSVC runtime DLL" FORCE)
FetchContent_Declare(raylib
    URL https://codeload.github.com/raysan5/raylib/tar.gz/refs/tags/5.5
    URL_HASH SHA256=aea98ecf5bc5c5e0b789a76de0083a21a70457050ea4cc2aec7566935f5e258e
)
FetchContent_Declare(imgui
    URL https://codeload.github.com/ocornut/imgui/tar.gz/refs/tags/v1.92.8
    URL_HASH SHA256=fecb33d33930e12ff53a34064e9d3a06c8f7c3e04408f14cd36c80e3faac863b
)
FetchContent_MakeAvailable(raylib imgui)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})
target_link_libraries(imgui PRIVATE opengl32)

add_executable(ValorantSensFinder WIN32
    src/win32_entry.cpp
    src/main.cpp
    src/raw_mouse.cpp
    src/benchmark.cpp
    src/adaptive_sens_tuner.cpp
    src/auto_calibration.cpp
    src/crosshair.cpp
    src/range_feedback.cpp
    src/calibration_report.cpp
    src/settings.cpp
    src/frame_sync.cpp
    src/customization_ui.cpp
    src/multisampling.cpp
)
target_compile_features(ValorantSensFinder PRIVATE cxx_std_20)
set_target_properties(ValorantSensFinder PROPERTIES CXX_EXTENSIONS OFF)
target_link_libraries(ValorantSensFinder PRIVATE raylib imgui comctl32 user32 opengl32)

install(TARGETS ValorantSensFinder RUNTIME DESTINATION . COMPONENT Portable)
install(FILES README.md DESTINATION . COMPONENT Portable)
install(FILES "${raylib_SOURCE_DIR}/LICENSE" DESTINATION licenses
        RENAME raylib.txt COMPONENT Portable)
install(FILES "${raylib_SOURCE_DIR}/src/external/glfw/LICENSE.md" DESTINATION licenses
        RENAME glfw.md COMPONENT Portable)
install(FILES "${imgui_SOURCE_DIR}/LICENSE.txt" DESTINATION licenses
        RENAME imgui.txt COMPONENT Portable)
