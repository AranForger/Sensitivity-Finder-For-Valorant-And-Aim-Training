#include "auto_calibration.h"
#include "calibration_report.h"
#include "crosshair.h"
#include "customization_ui.h"
#include "multisampling.h"
#include "range_feedback.h"
#include "settings.h"

#include <backends/imgui_impl_opengl3.h>
#include <imgui.h>
#include <raylib.h>
#include <rlgl.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <exception>
#include <numbers>
#include <stdexcept>
#include <utility>

void FinishGpuFrame();

namespace {
constexpr int WindowWidth = 1280;
constexpr int WindowHeight = 720;
constexpr double DegreesToRadians = std::numbers::pi / 180.0;
constexpr double HorizontalFov = 103.0;
constexpr float HalfWidth = 2700.0f;
constexpr float RoomHeight = 500.0f;
constexpr float FarWall = -2700.0f;
constexpr float RearWall = 2700.0f;
constexpr float WallThickness = 20.0f;

Camera3D MakeCamera(LookAngles look) {
    const auto direction = AimDirection(look);
    const double aspect = static_cast<double>(GetRenderWidth()) / std::max(1, GetRenderHeight());
    const float verticalFov = static_cast<float>(
        2.0 * std::atan(std::tan(HorizontalFov * DegreesToRadians / 2.0) / aspect) /
        DegreesToRadians);
    return {
        EyePosition,
        {direction.x * 100.0f, EyeHeight + direction.y * 100.0f, direction.z * 100.0f},
        {0.0f, 1.0f, 0.0f},
        verticalFov,
        CAMERA_PERSPECTIVE
    };
}

void DrawRoom(const RangePalette& palette) {
    constexpr float depth = RearWall - FarWall;
    constexpr float midZ = (RearWall + FarWall) / 2.0f;
    constexpr float width = 2.0f * HalfWidth;
    constexpr float halfHeight = RoomHeight / 2.0f;

    DrawCube({0.0f, -10.0f, midZ}, width, WallThickness, depth, palette.floor);
    DrawCube({0.0f, RoomHeight + 10.0f, midZ}, width, WallThickness, depth,
             ColorBrightness(palette.background, -0.32f));
    DrawCube({0.0f, halfHeight, FarWall - 10.0f}, width, RoomHeight, WallThickness,
             palette.background);
    DrawCube({0.0f, halfHeight, RearWall + 10.0f}, width, RoomHeight, WallThickness,
             ColorBrightness(palette.background, -0.12f));
    DrawCube({-HalfWidth - 10.0f, halfHeight, midZ}, WallThickness, RoomHeight, depth,
             palette.sideWalls);
    DrawCube({HalfWidth + 10.0f, halfHeight, midZ}, WallThickness, RoomHeight, depth,
             palette.sideWalls);

    const Color grid = palette.grid;
    for (int x = -2600; x <= 2600; x += 200) {
        DrawLine3D({static_cast<float>(x), 0.5f, FarWall},
                   {static_cast<float>(x), 0.5f, RearWall}, grid);
    }
    for (int z = -2600; z <= 2600; z += 200) {
        const float distance = static_cast<float>(z);
        DrawLine3D({-HalfWidth, 0.5f, distance}, {HalfWidth, 0.5f, distance}, grid);
    }
    for (int z = -2500; z <= 2500; z += 500) {
        const float distance = static_cast<float>(z);
        DrawCube({-HalfWidth + 6.0f, halfHeight, distance}, 12.0f, RoomHeight, 26.0f, grid);
        DrawCube({ HalfWidth - 6.0f, halfHeight, distance}, 12.0f, RoomHeight, 26.0f, grid);
    }

    // Center the visible bands at exactly 165 cm.
    DrawCube({0.0f, HeadHeight, FarWall + 2.0f}, width, 8.0f, 4.0f, palette.headLine);
    DrawCube({0.0f, HeadHeight, RearWall - 2.0f}, width, 8.0f, 4.0f, palette.headLine);
    DrawCube({-HalfWidth + 2.0f, HeadHeight, midZ}, 4.0f, 8.0f, depth, palette.headLine);
    DrawCube({ HalfWidth - 2.0f, HeadHeight, midZ}, 4.0f, 8.0f, depth, palette.headLine);
}

class DisplayMode {
public:
    DisplayMode()
        : position_(GetWindowPosition()), width_(GetScreenWidth()), height_(GetScreenHeight()) {
        // Re-enable GLFW's exclusive-fullscreen minimize on focus loss.
        glfwSetWindowAttrib(glfwGetCurrentContext(), GLFW_AUTO_ICONIFY, GLFW_TRUE);
    }

    void EnterFullscreen(int monitor) {
        const int width = GetMonitorWidth(monitor);
        const int height = GetMonitorHeight(monitor);
        if (width > 0 && height > 0) {
            SetWindowSize(width, height);
            ToggleFullscreen();
            // Win32 can limit the decorated window to the monitor's work area.
            SetWindowSize(width, height);
        } else {
            ToggleFullscreen();
        }
    }

    void Toggle(bool restoreWindow) {
        const bool fullscreen = IsWindowFullscreen();
        const bool borderless = IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE);
        if (restoreWindow) {
            if (fullscreen || borderless) {
                if (fullscreen) ToggleFullscreen();
                else ToggleBorderlessWindowed();
                SetWindowSize(width_, height_);
                SetWindowPosition(static_cast<int>(position_.x), static_cast<int>(position_.y));
            }
        } else if (borderless) {
            const int monitor = GetCurrentMonitor();
            ToggleBorderlessWindowed();
            EnterFullscreen(monitor);
        } else {
            if (!fullscreen) {
                position_ = GetWindowPosition();
                width_ = GetScreenWidth();
                height_ = GetScreenHeight();
            }
            ToggleBorderlessWindowed();
            ClearWindowState(FLAG_WINDOW_TOPMOST);
        }
    }

private:
    Vector2 position_{};
    int width_ = WindowWidth;
    int height_ = WindowHeight;
};

ImFont* ConfigureInterface() {
    ImGui::StyleColorsDark();
    auto& style = ImGui::GetStyle();
    style.FontSizeBase = 17.0f;
    auto& io = ImGui::GetIO();
    const auto fonts = WindowsFontsDirectory();
    const auto loadFont = [&](const char* name) -> ImFont* {
        std::error_code error;
        const auto path = fonts / name;
        if (fonts.empty() || !std::filesystem::is_regular_file(path, error)) return nullptr;
        return io.Fonts->AddFontFromFileTTF(DisplayPath(path).c_str());
    };
    io.FontDefault = loadFont("segoeui.ttf");
    if (!io.FontDefault) io.FontDefault = io.Fonts->AddFontDefaultVector();
    auto* emphasis = loadFont("segoeuib.ttf");
    style.WindowPadding = {16, 14};
    style.FramePadding = {9, 5};
    style.ItemSpacing = {9, 7};
    style.WindowRounding = 8.0f;
    style.FrameRounding = 4.0f;
    style.TabRounding = 4.0f;
    style.Colors[ImGuiCol_WindowBg] = {0.075f, 0.103f, 0.135f, 0.98f};
    style.Colors[ImGuiCol_TitleBgActive] = {0.11f, 0.17f, 0.22f, 1.0f};
    style.Colors[ImGuiCol_FrameBg] = {0.13f, 0.19f, 0.24f, 1.0f};
    style.Colors[ImGuiCol_Button] = {0.14f, 0.29f, 0.30f, 1.0f};
    style.Colors[ImGuiCol_ButtonHovered] = {0.20f, 0.43f, 0.42f, 1.0f};
    style.Colors[ImGuiCol_ButtonActive] = {0.22f, 0.50f, 0.47f, 1.0f};
    style.Colors[ImGuiCol_CheckMark] = {0.43f, 0.91f, 0.78f, 1.0f};
    style.Colors[ImGuiCol_SliderGrab] = {0.38f, 0.77f, 0.69f, 1.0f};
    style.Colors[ImGuiCol_Tab] = {0.12f, 0.22f, 0.26f, 1.0f};
    style.Colors[ImGuiCol_TabSelected] = {0.17f, 0.35f, 0.35f, 1.0f};
    style.Colors[ImGuiCol_TabHovered] = {0.20f, 0.43f, 0.42f, 1.0f};
    return emphasis ? emphasis : io.FontDefault;
}

void DrawFrameRate() {
    char label[24];
    std::snprintf(label, sizeof(label), "%d FPS", GetFPS());
    const auto size = ImGui::CalcTextSize(label);
    const float right = static_cast<float>(GetScreenWidth()) - 14.0f;
    constexpr float top = 14.0f;
    auto& draw = *ImGui::GetForegroundDrawList();
    draw.AddRectFilled({right - size.x - 16.0f, top}, {right, top + size.y + 10.0f},
                       IM_COL32(16, 23, 31, 216), 4.0f);
    draw.AddText({right - size.x - 8.0f, top + 5.0f}, IM_COL32(208, 230, 226, 255), label);
}

void DrawRoundCountdown(const AutoCalibration& calibration, double now, ImFont& font) {
    const int seconds = static_cast<int>(std::ceil(calibration.RestRemaining(now)));
    if (seconds <= 0) return;

    const auto* viewport = ImGui::GetMainViewport();
    const ImVec2 center{viewport->Pos.x + viewport->Size.x * 0.5f,
                        viewport->Pos.y + viewport->Size.y * 0.5f};
    ImGui::GetBackgroundDrawList()->AddRectFilled(
        viewport->Pos, {viewport->Pos.x + viewport->Size.x, viewport->Pos.y + viewport->Size.y},
        IM_COL32(9, 15, 23, 136));
    auto& draw = *ImGui::GetForegroundDrawList();
    const float digitSize = std::clamp(viewport->Size.y * 0.20f, 76.0f, 176.0f);
    char digit[16];
    std::snprintf(digit, sizeof(digit), "%d", seconds);
    const auto size = font.CalcTextSizeA(digitSize, FLT_MAX, 0.0f, digit);
    draw.AddText(&font, digitSize, {center.x - size.x * 0.5f, center.y - size.y * 0.5f},
                 IM_COL32(209, 255, 243, 255), digit);

    char label[48];
    std::snprintf(label, sizeof(label), "Round %d  |  Get ready", calibration.Tuner().RoundNumber());
    constexpr float labelSize = 23.0f;
    const auto labelWidth = font.CalcTextSizeA(labelSize, FLT_MAX, 0.0f, label).x;
    draw.AddText(&font, labelSize, {center.x - labelWidth * 0.5f, center.y - size.y * 0.5f - 38.0f},
                 IM_COL32(219, 234, 231, 255), label);
}

void BeginInterface(bool menu) {
    auto& io = ImGui::GetIO();
    io.DisplaySize = {static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())};
    io.DisplayFramebufferScale = {GetRenderWidth() / io.DisplaySize.x,
                                  GetRenderHeight() / io.DisplaySize.y};
    io.DeltaTime = std::max(GetFrameTime(), 0.001f);
    const auto mouse = GetMousePosition();
    io.MouseDrawCursor = menu && IsWindowFocused() && IsCursorOnScreen();
    io.AddMousePosEvent(menu ? mouse.x : -FLT_MAX, menu ? mouse.y : -FLT_MAX);
    io.AddMouseButtonEvent(0, menu && IsMouseButtonDown(MOUSE_BUTTON_LEFT));
    constexpr std::pair<ImGuiKey, int> editKeys[]{
        {ImGuiKey_Backspace, KEY_BACKSPACE}, {ImGuiKey_Delete, KEY_DELETE},
        {ImGuiKey_LeftArrow, KEY_LEFT}, {ImGuiKey_RightArrow, KEY_RIGHT},
        {ImGuiKey_UpArrow, KEY_UP}, {ImGuiKey_DownArrow, KEY_DOWN},
        {ImGuiKey_Home, KEY_HOME}, {ImGuiKey_End, KEY_END},
        {ImGuiKey_A, KEY_A}, {ImGuiKey_C, KEY_C}, {ImGuiKey_V, KEY_V}, {ImGuiKey_X, KEY_X},
        {ImGuiKey_Enter, KEY_ENTER}, {ImGuiKey_Escape, KEY_ESCAPE}
    };
    for (auto [imguiKey, raylibKey] : editKeys) {
        io.AddKeyEvent(imguiKey, menu && IsKeyDown(raylibKey));
    }
    io.AddKeyEvent(ImGuiMod_Ctrl, menu && (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)));
    io.AddKeyEvent(ImGuiMod_Shift, menu && (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)));
    for (int codepoint = GetCharPressed(); codepoint != 0; codepoint = GetCharPressed()) {
        if (menu) {
            io.AddInputCharacter(static_cast<unsigned int>(codepoint));
        }
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();
}

void EndInterface() {
    ImGui::Render();
    rlDrawRenderBatchActive();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void DrawRoundSummary(const CalibrationRound& round, int dpi) {
    ImGui::Text("Round %d: %.1f / 100  |  Bias %+.3f  |  TP %.2f bits/s",
                round.number, round.score, round.bias, round.throughput);
    ImGui::Text("Micro %.1f  |  Flick %.1f  |  Switch %.1f  |  Tracking %.1f",
                round.testScores[0], round.testScores[1], round.testScores[2], round.testScores[3]);
    if (!round.earlyStop && round.number < AdaptiveSensTuner::MaxRounds) {
        ImGui::Text("Next: %.4f sens (%.0f eDPI)", round.nextSensitivity, round.nextSensitivity * dpi);
    }
    ImGui::Text("MT outliers removed: %zu", round.removedTrials);
    if (round.highJitter || round.increaseLocked) {
        ImGui::TextColored({1.0f, 0.65f, 0.35f, 1.0f},
                           "%s (%.3f deg RMS)  |  increases locked",
                           round.highJitter ? "High Jitter" : "Jitter lock", round.trackingJitter);
    }
    if (round.backtracked) {
        ImGui::TextUnformatted("Backtracking");
    }
}

void DrawHud(const Benchmark& benchmark, const AutoCalibration& calibration,
             double sensitivity, LookAngles look, double now) {
    ImGui::SetNextWindowPos({18.0f, 18.0f});
    ImGui::SetNextWindowBgAlpha(0.88f);
    ImGui::Begin("Range HUD", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                 ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings);
    if (calibration.IsActive()) {
        const auto& tuner = calibration.Tuner();
        ImGui::Text("Auto-Calibration  |  Round %d / %d  |  Test %zu / 4",
                    tuner.RoundNumber(), AdaptiveSensTuner::MaxRounds, calibration.TestNumber());
        if (calibration.IsRoundCountdown()) {
            ImGui::TextUnformatted("Get ready");
        } else if (calibration.Status() == AutoCalibration::State::Resting) {
            ImGui::Text("Break: %.1f s  |  Next: %s", calibration.RestRemaining(now),
                        TestName(calibration.CurrentTest()));
        } else {
            ImGui::TextUnformatted(TestName(benchmark.Kind()));
        }
    } else {
        ImGui::TextUnformatted(benchmark.IsRunning() ? TestName(benchmark.Kind()) : "Practice Range");
    }
    ImGui::Separator();
    if (benchmark.IsRunning()) {
        if (benchmark.Kind() == Test::Tracking) {
            ImGui::Text("%s  |  %s  |  %.1f s remaining", ModeName(benchmark.Length()),
                        benchmark.IsReactive() ? "Reactive" : "Smooth", benchmark.SecondsLeft());
        } else {
            ImGui::Text("%s  |  %d / %d", ModeName(benchmark.Length()),
                        benchmark.Completed(), benchmark.Goal());
            ImGui::TextUnformatted(benchmark.AwaitingAnchor() ? "Click the center anchor"
                                                               : "Click the target");
        }
    } else if (!calibration.IsActive()) {
        ImGui::Text("Head-level line: %.0f cm", HeadHeight);
    }
    if (calibration.IsActive() && calibration.IsBlind()) {
        ImGui::TextDisabled("Blind Sens active");
    } else {
        ImGui::Text("Sensitivity: %.4f", sensitivity);
        ImGui::Text("Yaw: %+.2f  Pitch: %+.2f", look.yaw, look.pitch);
        if (calibration.IsActive() && !calibration.Tuner().History().empty()) {
            ImGui::Separator();
            DrawRoundSummary(calibration.Tuner().History().back(), calibration.Tuner().Dpi());
        }
    }
    ImGui::TextDisabled(benchmark.IsRunning() || calibration.IsActive()
                        ? "Esc / Tab: pause" : "R: reset view  |  Esc / Tab: pause");
    ImGui::End();
}

void DrawResult(const Benchmark& benchmark) {
    const auto& result = benchmark.Result();
    ImGui::SeparatorText("Result");
    ImGui::Text("%s  |  %s", TestName(benchmark.Kind()), ModeName(benchmark.Length()));
    if (benchmark.Kind() == Test::Tracking) {
        ImGui::Text("Time on target: %.1f%%  |  Jitter: %.3f deg",
                    result.timeOnTarget * 100.0, result.jitter);
        ImGui::Text("Perpendicular jitter: %.3f deg RMS (%.1f s sampled)",
                    result.perpendicularJitter, result.jitterSeconds);
        if (result.reactions > 0) {
            ImGui::Text("Mean reaction: %.0f ms (%d reversals)",
                        result.meanReaction * 1000.0, result.reactions);
        } else {
            ImGui::TextUnformatted("Mean reaction: no responses measured");
        }
    } else {
        ImGui::Text("Mean movement time: %.0f ms  |  Corrected overshoots: %d",
                    result.aim.meanMt * 1000.0, result.correctedOvershoots);
        ImGui::Text("Effective width: %.3f deg  |  Throughput: %.2f bits/s",
                    result.aim.effectiveWidth, result.aim.throughput);
        if (benchmark.Kind() == Test::Switch) {
            ImGui::Text("Mean time between hits: %.0f ms  |  Path efficiency: %.1f%%",
                        result.meanSwitchDelay * 1000.0, result.pathEfficiency * 100.0);
        } else if (benchmark.Kind() == Test::Micro) {
            ImGui::Text("RMS deviation from aim path: %.3f deg", result.jitter);
        }
    }
}

constexpr int ResumeAction = 4;
constexpr int FreeAimAction = 5;
constexpr int StartCalibrationAction = 6;
constexpr int MainMenuAction = 7;
constexpr int ApplyRecommendationAction = 8;
constexpr int ExitAction = 9;

enum class Screen { MainMenu, Playing, Paused };

int DrawPauseMenu(const Benchmark& benchmark, const AutoCalibration& calibration, double now) {
    ImGui::SetNextWindowPos({GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f},
                            ImGuiCond_Always, {0.5f, 0.5f});
    ImGui::SetNextWindowSize({360.0f, 0.0f});
    int action = -1;
    ImGui::Begin("Paused", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);
    const auto& tuner = calibration.Tuner();
    if (calibration.IsActive()) {
        ImGui::Text("Auto-Calibration  |  Round %d / %d",
                    tuner.RoundNumber(), AdaptiveSensTuner::MaxRounds);
        if (calibration.IsRoundCountdown()) {
            ImGui::Text("Countdown remaining: %.1f s", calibration.RestRemaining(now));
        } else if (calibration.Status() == AutoCalibration::State::Resting) {
            ImGui::Text("Break remaining: %.1f s", calibration.RestRemaining(now));
        } else {
            ImGui::TextUnformatted(TestName(benchmark.Kind()));
        }
    } else {
        ImGui::TextUnformatted(benchmark.IsRunning() ? TestName(benchmark.Kind()) : "Free aim");
    }
    ImGui::Separator();
    const ImVec2 size{ImGui::GetContentRegionAvail().x, 34.0f};
    if (ImGui::Button("Continue", size)) action = ResumeAction;
    if (ImGui::Button("Main Menu", size)) action = MainMenuAction;
    if (ImGui::Button("Exit Game", size)) action = ExitAction;
    ImGui::TextDisabled("Esc: continue");
    ImGui::End();
    return action;
}

int DrawPracticeMenu(const Benchmark& benchmark, Mode& mode, double& sensitivity,
                     CalibrationSetup& setup) {
    int action = -1;
    if (ImGui::RadioButton("Full", mode == Mode::Full)) {
        mode = Mode::Full;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Condensed", mode == Mode::Condensed)) {
        mode = Mode::Condensed;
    }
    ImGui::SameLine();
    ImGui::TextDisabled("M: toggle");
    float slider = static_cast<float>(sensitivity);
    ImGui::SetNextItemWidth(250.0f);
    if (ImGui::SliderFloat("Sensitivity (next test)", &slider, 0.01f, 10.0f, "%.2f",
                           ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoInput)) {
        sensitivity = slider;
    }
    ImGui::Separator();
    const bool full = mode == Mode::Full;
    const char* lengths[]{full ? "25 targets" : "10 targets", full ? "25 trials" : "10 trials",
                          full ? "10 clusters / 30 targets" : "4 clusters / 12 targets",
                          full ? "10 s Smooth + 10 s Reactive" : "5 s Smooth + 5 s Reactive"};
    for (int i = 0; i < 4; ++i) {
        ImGui::PushID(i);
        if (ImGui::Button(TestName(BenchmarkOrder[i]), {245.0f, 29.0f})) {
            action = i;
        }
        ImGui::SameLine();
        ImGui::Text("%d  |  %s", i + 1, lengths[i]);
        ImGui::PopID();
    }
    ImGui::Separator();
    if (ImGui::Button("Free aim")) {
        action = FreeAimAction;
    }
    ImGui::SeparatorText("Auto-Calibration");
    ImGui::SetNextItemWidth(145.0f);
    ImGui::InputInt("Mouse DPI", &setup.dpi, 50, 200, ImGuiInputTextFlags_CharsDecimal);
    ImGui::Checkbox("Blind Sens", &setup.blind);
    ImGui::SetItemTooltip("Randomizes the starting sensitivity and hides values and interim scores "
                          "until the final report.");
    if (!setup.blind) {
        ImGui::Checkbox("Start at suggested 280 eDPI", &setup.suggested);
        if (!setup.suggested) {
            ImGui::SetNextItemWidth(145.0f);
            ImGui::InputDouble("Starting sensitivity", &setup.customSensitivity, 0.01, 0.10, "%.5f",
                               ImGuiInputTextFlags_CharsDecimal);
        }
    }
    const bool valid = setup.dpi > 0 && (setup.blind || setup.suggested ||
        (std::isfinite(setup.customSensitivity) && setup.customSensitivity > 0.0));
    if (valid && !setup.blind) {
        const double start = AdaptiveSensTuner::ClampSensitivity(setup.dpi, setup.Sensitivity());
        ImGui::Text("Start: %.4f sens  |  %.0f eDPI", start, start * setup.dpi);
    } else if (!valid) {
        ImGui::TextUnformatted("Enter a positive DPI and starting sensitivity.");
    }
    ImGui::BeginDisabled(!valid);
    if (ImGui::Button("Start Auto-Calibration")) {
        action = StartCalibrationAction;
    }
    ImGui::EndDisabled();
    if (benchmark.IsFinished()) {
        DrawResult(benchmark);
    }
    if (benchmark.TelemetryFailed()) {
        ImGui::TextColored({1.0f, 0.50f, 0.50f, 1.0f}, "Could not write telemetry");
    } else if (!benchmark.TelemetryPath().empty()) {
        ImGui::TextUnformatted("Telemetry CSV");
        ImGui::PushTextWrapPos();
        ImGui::TextUnformatted(benchmark.TelemetryPath().c_str());
        ImGui::PopTextWrapPos();
    }
    return action;
}

int DrawMenu(const Benchmark& benchmark, const AutoCalibration& calibration,
             UserSettings& settings, RangeFeedback& feedback, CalibrationReport& report, Mode& mode,
             int activeSamples) {
    const bool finished = calibration.Status() == AutoCalibration::State::Complete;
    const float width = std::min(finished ? 870.0f : 640.0f, GetScreenWidth() - 36.0f);
    ImGui::SetNextWindowPos({GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f},
                            ImGuiCond_Always, {0.5f, 0.5f});
    ImGui::SetNextWindowSize({width, 0.0f});
    ImGui::SetNextWindowSizeConstraints({width, 0.0f}, {width, GetScreenHeight() - 36.0f});
    const char* title = finished ? "Calibration report###AimMenu" : "Aim Benchmarks###AimMenu";
    int action = -1;
    if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings)) {
        if (finished) {
            report.Draw(calibration.Tuner());
            ImGui::Spacing();
            if (ImGui::Button("Use in practice")) action = ApplyRecommendationAction;
            ImGui::SameLine();
            if (ImGui::Button("Main Menu")) action = MainMenuAction;
            ImGui::SameLine();
            if (ImGui::Button("Exit Game")) action = ExitAction;
        } else if (ImGui::BeginTabBar("Practice settings")) {
            if (ImGui::BeginTabItem("Benchmarks")) {
                action = DrawPracticeMenu(benchmark, mode, settings.sensitivity, settings.calibration);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Crosshair")) {
                settings.crosshair.DrawSettings();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Appearance")) {
                DrawAppearanceSettings(settings.palette);
                ImGui::SeparatorText("Graphics");
                const char* options[]{"Off", "2x", "4x", "8x"};
                const auto choice = std::find(MsaaOptions.begin(), MsaaOptions.end(), settings.msaa);
                int index = static_cast<int>(choice - MsaaOptions.begin());
                ImGui::SetNextItemWidth(130.0f);
                if (ImGui::Combo("Anti-aliasing (MSAA)", &index, options, IM_ARRAYSIZE(options))) {
                    settings.msaa = MsaaOptions[index];
                }
                if (activeSamples != settings.msaa && !ImGui::IsItemEdited()) {
                    ImGui::TextDisabled(activeSamples > 0 ? "Active on this GPU: %dx" :
                                                               "Active on this GPU: Off", activeSamples);
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Audio")) {
                DrawAudioSettings(settings.audio, feedback);
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
            if (ImGui::Button("Exit Game")) action = ExitAction;
            ImGui::SameLine();
            ImGui::Checkbox("Low Latency", &settings.lowLatency);
            ImGui::SetItemTooltip("Disables VSync and waits for OpenGL rendering before the next frame.\n"
                                 "May cause tearing or reduce FPS.");
            ImGui::TextDisabled("F11: borderless / fullscreen   |   F10: windowed");
        }
    }
    ImGui::End();
    return action;
}

void RunRange() {
    const auto settingsPath = ApplicationDirectory() / "settings.ini";
    auto settings = LoadSettings(settingsPath);
    DisplayMode display;
    const auto updateVsync = [&] {
        if (settings.lowLatency) ClearWindowState(FLAG_VSYNC_HINT);
        else SetWindowState(FLAG_VSYNC_HINT);
    };
    updateVsync();
    display.EnterFullscreen(GetCurrentMonitor());
    EnableCursor();
    RawMouse mouse(GetWindowHandle());
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    auto* countdownFont = ConfigureInterface();
    if (!ImGui_ImplOpenGL3_Init("#version 330")) {
        ImGui::DestroyContext();
        throw std::runtime_error("Could not initialize the Dear ImGui OpenGL renderer.");
    }

    rlSetClipPlanes(5.0, 8000.0);  // Room geometry is specified in centimeters.
    Benchmark benchmark;
    AutoCalibration calibration;
    auto& setup = settings.calibration;
    auto& crosshair = settings.crosshair;
    CalibrationReport report;
    RangeFeedback feedback;
    MultisampleTarget msaa;
    feedback.SetAudio(settings.audio);
    Mode mode = Mode::Full;
    auto& sensitivity = settings.sensitivity;
    double activeSensitivity = sensitivity;
    Screen screen = Screen::MainMenu;
    int action = -1;
    const auto applySensitivity = [&](double value) {
        mouse.ResetLook();
        activeSensitivity = value;
        mouse.SetSensitivity(value);
    };

    while (!WindowShouldClose()) {
        const double now = InputTime();
        const bool playing = screen == Screen::Playing;
        for (const auto& sample : mouse.Samples()) {
            const bool canShoot = benchmark.IsRunning() ||
                (!calibration.IsActive() && !benchmark.IsFinished());
            const auto hit = benchmark.Observe(sample);
            if (playing && sample.clicked && canShoot) {
                feedback.Shot(now, hit);
            }
        }
        mouse.ClearSamples();
        bool pauseRequested = playing && (!mouse.IsCaptured() || !IsWindowFocused());
        int displayKey = KEY_NULL;
        for (int key = GetKeyPressed(); key != 0; key = GetKeyPressed()) {
            if (key == KEY_F11 || key == KEY_F10) {
                displayKey = key;
                pauseRequested = playing;
            } else if (screen == Screen::Paused) {
                if (key == KEY_ESCAPE || key == KEY_ENTER) action = ResumeAction;
            } else if (screen == Screen::MainMenu) {
                if (ImGui::GetIO().WantTextInput) {
                    continue;
                }
                if (calibration.Status() != AutoCalibration::State::Idle) continue;
                switch (key) {
                    case KEY_ONE: case KEY_TWO: case KEY_THREE: case KEY_FOUR:
                        action = key - KEY_ONE;
                        break;
                    case KEY_M: mode = mode == Mode::Full ? Mode::Condensed : Mode::Full; break;
                    case KEY_UP: sensitivity = std::min(sensitivity + 0.01, 10.0); break;
                    case KEY_DOWN: sensitivity = std::max(sensitivity - 0.01, 0.01); break;
                    default: break;
                }
            } else if (key == KEY_ESCAPE ||
                       (key == KEY_TAB && !IsKeyDown(KEY_LEFT_ALT) && !IsKeyDown(KEY_RIGHT_ALT))) {
                pauseRequested = true;
            } else if (!benchmark.IsRunning() && !calibration.IsActive() && key == KEY_R) {
                mouse.ResetLook();
            }
        }
        benchmark.Advance(now);
        if (playing && !pauseRequested && calibration.Advance(benchmark, now)) {
            applySensitivity(calibration.Tuner().CurrentSensitivity());
        }

        const bool finished = calibration.Status() == AutoCalibration::State::Complete ||
            (!calibration.IsActive() && benchmark.IsFinished() &&
             (pauseRequested || !feedback.HasImpact(now)));
        if (playing && (pauseRequested || finished)) {
            if (calibration.IsActive()) {
                calibration.SetPaused(benchmark, true, now);
            } else {
                benchmark.SetPaused(true, now);
            }
            screen = finished ? Screen::MainMenu : Screen::Paused;
            mouse.SetCaptured(false);
            EnableCursor();
        } else if (!playing && action >= 0 && displayKey == KEY_NULL && IsWindowFocused()) {
            if (action == ExitAction) {
                break;
            }
            if (action == MainMenuAction || action == ApplyRecommendationAction) {
                if (action == ApplyRecommendationAction) {
                    if (const auto* recommended = calibration.Tuner().Recommendation()) {
                        sensitivity = recommended->sensitivity;
                    }
                }
                calibration.Cancel(benchmark);
                benchmark.Stop();
                screen = Screen::MainMenu;
            } else {
                if (action == StartCalibrationAction) {
                    report.Reset();
                    calibration.Start(benchmark, setup.dpi, setup.blind ? 0.0 : setup.Sensitivity(),
                                      setup.blind, now);
                    mouse.ResetLook();
                } else if (action == ResumeAction) {
                    if (calibration.IsActive()) {
                        calibration.SetPaused(benchmark, false, now);
                    } else {
                        benchmark.SetPaused(false, now);
                    }
                } else {
                    applySensitivity(sensitivity);
                    if (action == FreeAimAction) {
                        benchmark.Stop();
                    } else {
                        benchmark.Start(BenchmarkOrder[action], mode, activeSensitivity, now);
                    }
                }
                DisableCursor();
                mouse.SetCaptured(true);
                screen = Screen::Playing;
            }
        }
        action = -1;
        if (displayKey != KEY_NULL) {
            display.Toggle(displayKey == KEY_F10);
            updateVsync();
            EnableCursor();
        }
        const bool menu = screen != Screen::Playing;
        if (menu) {
            // In menus, ImGui draws the pointer in the window's client area.
            if (IsWindowFocused() && !IsCursorHidden()) HideCursor();
            else if (!IsWindowFocused() && IsCursorHidden()) ShowCursor();
        }
        if (calibration.Status() != AutoCalibration::State::Resting) {
            feedback.UpdateCountdown(0.0);
        } else if (!menu) {
            feedback.UpdateCountdown(calibration.RestRemaining(now));
        }

        const auto look = mouse.GetLook();
        BeginDrawing();
        const bool multisampled = msaa.Begin(GetRenderWidth(), GetRenderHeight(), settings.msaa);
        ClearBackground(ColorBrightness(settings.palette.background, -0.5f));
        BeginMode3D(MakeCamera(look));
        DrawRoom(settings.palette);
        benchmark.Draw(settings.palette);
        feedback.DrawImpacts(now, EyePosition, settings.palette.impact);
        EndMode3D();
        if (multisampled) msaa.Resolve();
        BeginInterface(menu);
        if (screen == Screen::Paused) {
            action = DrawPauseMenu(benchmark, calibration, now);
        } else if (screen == Screen::MainMenu) {
            const bool previousLowLatency = settings.lowLatency;
            action = DrawMenu(benchmark, calibration, settings, feedback, report, mode, msaa.Samples());
            if (previousLowLatency != settings.lowLatency) updateVsync();
        } else {
            if (calibration.Status() != AutoCalibration::State::Resting) {
                crosshair.Draw(*ImGui::GetForegroundDrawList(),
                               {GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f});
            }
            DrawHud(benchmark, calibration, activeSensitivity, look, now);
            if (calibration.IsRoundCountdown()) DrawRoundCountdown(calibration, now, *countdownFont);
        }
        DrawFrameRate();
        EndInterface();
        if (!menu && settings.lowLatency) FinishGpuFrame();
        EndDrawing();
    }

    benchmark.Stop();
    if (!SaveSettings(settingsPath, settings)) {
        TraceLog(LOG_WARNING, "Settings could not be saved to %s", DisplayPath(settingsPath).c_str());
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();
}
}

int RunApplication() {
    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(WindowWidth, WindowHeight, "Valorant Sensitivity Finder - Practice Range");
    if (!IsWindowReady()) {
        return 1;
    }
    SetExitKey(KEY_NULL);

    int result = 0;
    try {
        RunRange();
    } catch (const std::exception& error) {
        TraceLog(LOG_ERROR, "%s", error.what());
        result = 1;
    }
    CloseWindow();
    return result;
}
