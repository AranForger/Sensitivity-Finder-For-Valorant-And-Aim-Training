#pragma once

#include "crosshair.h"
#include "customization.h"

#include <filesystem>
#include <string>

struct CalibrationSetup {
    int dpi = 800;
    double customSensitivity = 0.4;
    bool suggested = true;
    bool blind = false;

    double Sensitivity() const;
};

struct UserSettings {
    CalibrationSetup calibration;
    double sensitivity = 0.4;
    Crosshair crosshair;
    RangePalette palette;
    AudioPreferences audio;
    bool lowLatency = false;
    int msaa = 4;
};

const std::filesystem::path& ApplicationDirectory();
std::filesystem::path WindowsFontsDirectory();
std::string DisplayPath(const std::filesystem::path& path);
UserSettings LoadSettings(const std::filesystem::path& path);
bool SaveSettings(const std::filesystem::path& path, const UserSettings& settings);
