#pragma once

#include <vector>

inline constexpr double ValorantDegreesPerCount = 0.07;

struct LookAngles {
    double yaw = 0.0;
    double pitch = 0.0;

    void ApplyRawMotion(long dx, long dy, double sensitivity);
};

struct MouseSample {
    LookAngles look;
    double time;
    bool clicked;
};

double InputTime();

class RawMouse {
public:
    explicit RawMouse(void* nativeWindow);
    ~RawMouse();

    RawMouse(const RawMouse&) = delete;
    RawMouse& operator=(const RawMouse&) = delete;

    void SetCaptured(bool captured);
    bool IsCaptured() const { return captured_; }
    void SetSensitivity(double sensitivity) { sensitivity_ = sensitivity; }
    void ResetLook() { angles_ = {}; }
    LookAngles GetLook() const { return angles_; }
    const std::vector<MouseSample>& Samples() const { return samples_; }
    void ClearSamples() { samples_.clear(); }

private:
    struct WindowSubclass;

    void* window_;
    bool captured_ = false;
    double sensitivity_ = 1.0;
    LookAngles angles_;
    std::vector<MouseSample> samples_;
};
