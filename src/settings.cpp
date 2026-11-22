#include "settings.h"

#include "adaptive_sens_tuner.h"
#include "multisampling.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NOGDI
#define NOUSER
#include <windows.h>

#include <array>
#include <charconv>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <locale>
#include <string_view>
#include <system_error>
#include <vector>

namespace {
std::string_view Trim(std::string_view value) {
    constexpr std::string_view spaces = " \t\r\n";
    const auto first = value.find_first_not_of(spaces);
    if (first == std::string_view::npos) {
        return {};
    }
    return value.substr(first, value.find_last_not_of(spaces) - first + 1);
}

template <typename Number>
bool ReadNumber(std::string_view text, Number& value) {
    if (text.empty()) {
        return false;
    }
    const auto* end = text.data() + text.size();
    const auto parsed = std::from_chars(text.data(), end, value);
    return parsed.ec == std::errc{} && parsed.ptr == end;
}

void ReadPositive(std::string_view text, double& field,
                  double max = std::numeric_limits<double>::max()) {
    double value = 0.0;
    if (ReadNumber(text, value) && std::isfinite(value) && value > 0.0 && value <= max) {
        field = value;
    }
}

void ReadInt(std::string_view text, int& field, int low, int high) {
    int value = 0;
    if (ReadNumber(text, value) && value >= low && value <= high) {
        field = value;
    }
}

void ReadBool(std::string_view text, bool& field) {
    if (text == "1" || text == "true") field = true;
    if (text == "0" || text == "false") field = false;
}

void ReadColor(std::string_view text, Color& color) {
    if (text.starts_with('#')) text.remove_prefix(1);
    if (text.size() != 6) return;
    unsigned value = 0;
    const auto* end = text.data() + text.size();
    const auto parsed = std::from_chars(text.data(), end, value, 16);
    if (parsed.ec == std::errc{} && parsed.ptr == end) {
        color = {static_cast<unsigned char>(value >> 16),
                 static_cast<unsigned char>((value >> 8) & 0xffu),
                 static_cast<unsigned char>(value & 0xffu), 255};
    }
}

void WriteSensitivity(std::ostream& output, double value) {
    std::array<char, 32> buffer{};
    const auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
    output.write(buffer.data(), result.ptr - buffer.data());
}
}

double CalibrationSetup::Sensitivity() const {
    return suggested ? AdaptiveSensTuner::SuggestedEdpi / dpi : customSensitivity;
}

const std::filesystem::path& ApplicationDirectory() {
    static const auto directory = [] {
        std::vector<wchar_t> buffer(260);
        for (;;) {
            const auto size = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
            if (size == 0) {
                throw std::system_error(static_cast<int>(GetLastError()), std::system_category(),
                                        "Could not locate the executable");
            }
            if (size < buffer.size()) {
                return std::filesystem::path(std::wstring_view(buffer.data(), size)).parent_path();
            }
            buffer.resize(buffer.size() * 2);
        }
    }();
    return directory;
}

std::string DisplayPath(const std::filesystem::path& path) {
    const auto utf8 = path.u8string();
    return {utf8.begin(), utf8.end()};
}

std::filesystem::path WindowsFontsDirectory() {
    std::array<wchar_t, MAX_PATH> buffer{};
    const auto size = GetWindowsDirectoryW(buffer.data(), static_cast<UINT>(buffer.size()));
    if (size == 0 || size >= buffer.size()) {
        return {};
    }
    return std::filesystem::path(std::wstring_view(buffer.data(), size)) / "Fonts";
}

UserSettings LoadSettings(const std::filesystem::path& path) {
    UserSettings settings;
    std::ifstream input(path);
    std::string section;
    std::string line;
    while (std::getline(input, line)) {
        auto text = std::string_view(line);
        if (text.starts_with("\xef\xbb\xbf")) {
            text.remove_prefix(3);
        }
        text = Trim(text);
        if (text.starts_with('[') && text.ends_with(']')) {
            section = Trim(text.substr(1, text.size() - 2));
            continue;
        }
        const auto separator = text.find('=');
        if (separator == std::string_view::npos) {
            continue;
        }
        const auto key = Trim(text.substr(0, separator));
        const auto value = Trim(text.substr(separator + 1));
        if (section == "mouse") {
            if (key == "dpi") ReadInt(value, settings.calibration.dpi, 1, std::numeric_limits<int>::max());
            else if (key == "sensitivity") ReadPositive(value, settings.sensitivity, AdaptiveSensTuner::MaxEdpi);
            else if (key == "calibration_sensitivity") ReadPositive(value, settings.calibration.customSensitivity);
            else if (key == "use_suggested_edpi") ReadBool(value, settings.calibration.suggested);
            else if (key == "blind_sens") ReadBool(value, settings.calibration.blind);
        } else if (section == "video") {
            if (key == "low_latency") ReadBool(value, settings.lowLatency);
            if (key == "msaa") {
                int samples = -1;
                if (ReadNumber(value, samples)) {
                    for (const auto option : MsaaOptions) {
                        if (option == samples) settings.msaa = samples;
                    }
                }
            }
        } else if (section == "palette") {
            for (const auto& field : PaletteFields) {
                if (key == field.key) {
                    ReadColor(value, settings.palette.*field.value);
                    break;
                }
            }
        } else if (section == "audio") {
            auto& audio = settings.audio;
            if (key == "master") ReadInt(value, audio.master, 0, 100);
            else if (key == "shot") ReadInt(value, audio.shot, 0, 100);
            else if (key == "hit") ReadInt(value, audio.hit, 0, 100);
            else if (key == "countdown") ReadInt(value, audio.countdown, 0, 100);
            else if (key == "muted") ReadBool(value, audio.muted);
        } else if (section == "crosshair") {
            auto& crosshair = settings.crosshair;
            if (key == "color") ReadInt(value, crosshair.color, 0, Crosshair::ColorCount - 1);
            else if (key == "length") ReadInt(value, crosshair.length, 0, Crosshair::MaxLength);
            else if (key == "thickness") ReadInt(value, crosshair.thickness, 1, Crosshair::MaxThickness);
            else if (key == "offset") ReadInt(value, crosshair.offset, 0, Crosshair::MaxOffset);
            else if (key == "center_dot") ReadBool(value, crosshair.centerDot);
            else if (key == "outlines") ReadBool(value, crosshair.outlines);
        }
    }
    return settings;
}

bool SaveSettings(const std::filesystem::path& path, const UserSettings& settings) {
    auto temporary = path;
    temporary += L"." + std::to_wstring(GetCurrentProcessId()) + L".tmp";
    std::ofstream output(temporary, std::ios::trunc);
    output.imbue(std::locale::classic());
    output << "[mouse]\ndpi=" << settings.calibration.dpi << "\nsensitivity=";
    WriteSensitivity(output, settings.sensitivity);
    output << "\ncalibration_sensitivity=";
    WriteSensitivity(output, settings.calibration.customSensitivity);
    output << "\nuse_suggested_edpi=" << settings.calibration.suggested
           << "\nblind_sens=" << settings.calibration.blind
           << "\n\n[crosshair]\ncolor=" << settings.crosshair.color
           << "\nlength=" << settings.crosshair.length
           << "\nthickness=" << settings.crosshair.thickness
           << "\noffset=" << settings.crosshair.offset
           << "\ncenter_dot=" << settings.crosshair.centerDot
           << "\noutlines=" << settings.crosshair.outlines
           << "\n\n[video]\nlow_latency=" << settings.lowLatency
           << "\nmsaa=" << settings.msaa
           << "\n\n[audio]\nmaster=" << settings.audio.master
           << "\nshot=" << settings.audio.shot
           << "\nhit=" << settings.audio.hit
           << "\ncountdown=" << settings.audio.countdown
           << "\nmuted=" << settings.audio.muted
           << "\n\n[palette]\n" << std::hex << std::setfill('0');
    for (const auto& field : PaletteFields) {
        const auto color = settings.palette.*field.value;
        const auto value = (static_cast<unsigned>(color.r) << 16) |
                           (static_cast<unsigned>(color.g) << 8) | color.b;
        output << field.key << "=#" << std::setw(6) << value << '\n';
    }
    output.close();
    if (output && MoveFileExW(temporary.c_str(), path.c_str(),
                              MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        return true;
    }
    std::error_code error;
    std::filesystem::remove(temporary, error);
    return false;
}
