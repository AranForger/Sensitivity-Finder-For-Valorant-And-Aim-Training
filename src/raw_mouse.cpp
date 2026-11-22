#include "raw_mouse.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commctrl.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <stdexcept>
#include <system_error>

namespace {
constexpr USHORT GenericDesktop = 0x01;
constexpr USHORT MouseUsage = 0x02;
constexpr double PitchLimit = 89.0;

void RegisterMouse(HWND window) {
    const RAWINPUTDEVICE mouse{GenericDesktop, MouseUsage, RIDEV_INPUTSINK, window};
    if (!RegisterRawInputDevices(&mouse, 1, sizeof(mouse))) {
        throw std::system_error(static_cast<int>(GetLastError()), std::system_category(),
                                "Could not register the Raw Input mouse");
    }
}
}

struct RawMouse::WindowSubclass {
    static LRESULT CALLBACK HandleMessage(HWND window, UINT message, WPARAM wParam,
                                          LPARAM lParam, UINT_PTR id, DWORD_PTR data) {
        auto& mouse = *reinterpret_cast<RawMouse*>(data);
        if (message == WM_INPUT && mouse.captured_ && GET_RAWINPUT_CODE_WPARAM(wParam) == RIM_INPUT) {
            RAWINPUT input{};
            UINT size = sizeof(input);
            const UINT read = GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT,
                                              &input, &size, sizeof(RAWINPUTHEADER));
            if (read == sizeof(input) && input.header.dwType == RIM_TYPEMOUSE) {
                const auto& packet = input.data.mouse;
                const bool moved = !(packet.usFlags & MOUSE_MOVE_ABSOLUTE) &&
                    (packet.lLastX != 0 || packet.lLastY != 0);
                const bool clicked = (packet.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN) != 0;
                if (moved) {
                    mouse.angles_.ApplyRawMotion(packet.lLastX, packet.lLastY, mouse.sensitivity_);
                }
                if (moved || clicked) {
                    mouse.samples_.push_back({mouse.angles_, InputTime(), clicked});
                }
            }
        } else if (message == WM_KILLFOCUS) {
            mouse.captured_ = false;
        } else if (message == WM_NCDESTROY) {
            RemoveWindowSubclass(window, HandleMessage, id);
            mouse.window_ = nullptr;
        }

        // Let the original window procedure process WM_INPUT and perform system cleanup.
        return DefSubclassProc(window, message, wParam, lParam);
    }
};

RawMouse::RawMouse(void* nativeWindow) : window_(nativeWindow) {
    samples_.reserve(256);
    const auto window = static_cast<HWND>(window_);
    const auto id = reinterpret_cast<UINT_PTR>(this);
    if (!SetWindowSubclass(window, WindowSubclass::HandleMessage, id,
                           reinterpret_cast<DWORD_PTR>(this))) {
        throw std::runtime_error("Could not subclass the range window for Raw Input.");
    }

    try {
        RegisterMouse(window);
    } catch (...) {
        RemoveWindowSubclass(window, WindowSubclass::HandleMessage, id);
        throw;
    }
}

void RawMouse::SetCaptured(bool captured) {
    if (captured) {
        // Raylib's GLFW cursor transition can replace or remove this registration.
        RegisterMouse(static_cast<HWND>(window_));
    }
    captured_ = captured;
}

double InputTime() {
    using Clock = std::chrono::steady_clock;
    static const auto origin = Clock::now();
    return std::chrono::duration<double>(Clock::now() - origin).count();
}

RawMouse::~RawMouse() {
    const RAWINPUTDEVICE mouse{GenericDesktop, MouseUsage, RIDEV_REMOVE, nullptr};
    RegisterRawInputDevices(&mouse, 1, sizeof(mouse));
    if (window_) {
        RemoveWindowSubclass(static_cast<HWND>(window_), WindowSubclass::HandleMessage,
                             reinterpret_cast<UINT_PTR>(this));
    }
}

void LookAngles::ApplyRawMotion(long dx, long dy, double sensitivity) {
    const double degreesPerCount = sensitivity * ValorantDegreesPerCount;
    yaw = std::remainder(yaw - dx * degreesPerCount, 360.0);
    pitch = std::clamp(pitch - dy * degreesPerCount, -PitchLimit, PitchLimit);
}
