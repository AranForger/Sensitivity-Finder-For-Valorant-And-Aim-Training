#include "calibration_report.h"

#include "raw_mouse.h"
#include "settings.h"

#include <imgui.h>
#include <raylib.h>

#include <algorithm>
#include <array>
#include <cfloat>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <locale>
#include <sstream>
#include <stdexcept>

namespace {
constexpr ImVec4 Accent{0.39f, 0.90f, 0.76f, 1.0f};
constexpr ImVec4 Warm{1.0f, 0.69f, 0.43f, 1.0f};

std::string FormatSensitivity(double value) {
    std::ostringstream text;
    text.imbue(std::locale::classic());
    text << std::fixed << std::setprecision(6) << value;
    return text.str();
}

void PlotHistory(const char* label, const float* values, int count, ImVec4 color) {
    const auto [low, high] = std::minmax_element(values, values + count);
    const float padding = std::max((*high - *low) * 0.15f, std::max(0.001f, *high * 0.03f));
    ImGui::TextUnformatted(label);
    ImGui::PushStyleColor(ImGuiCol_PlotLines, color);
    ImGui::PlotLines("##rounds", values, count, 0, nullptr,
                     *low - padding, *high + padding, {-FLT_MIN, 83.0f});
    ImGui::PopStyleColor();
    ImGui::TextDisabled("Round 1 to %d   |   %.3f to %.3f", count, *low, *high);
}

void DrawSkillScores(const CalibrationRound& round) {
    constexpr const char* skills[]{"Micro", "Flick", "Target Switching", "Tracking"};
    if (ImGui::BeginTable("Skills", 2, ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Skill", ImGuiTableColumnFlags_WidthFixed, 147.0f);
        ImGui::TableSetupColumn("Score", ImGuiTableColumnFlags_WidthStretch);
        for (std::size_t i = 0; i < round.testScores.size(); ++i) {
            ImGui::PushID(static_cast<int>(i));
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(skills[i]);
            ImGui::TableNextColumn();
            char label[32];
            std::snprintf(label, sizeof(label), "%.1f / 100", round.testScores[i]);
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, Accent);
            ImGui::ProgressBar(static_cast<float>(round.testScores[i] / 100.0),
                               {-FLT_MIN, 0.0f}, label);
            ImGui::PopStyleColor();
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
}

void DrawFindings(const CalibrationRound& round, const std::vector<CalibrationRound>& history) {
    const auto& flick = round.aim[1];
    if (flick.retained > 0) {
        if (flick.bias < -0.08) {
            ImGui::BulletText("Wide flicks tended to fall short (bias %+.3f).", flick.bias);
        } else if (flick.bias > 0.08) {
            ImGui::BulletText("Wide flicks tended to overshoot (bias %+.3f).", flick.bias);
        } else {
            ImGui::BulletText("Wide flicks showed little directional bias (%+.3f).", flick.bias);
        }
    }

    const auto& micro = round.aim[0];
    if (micro.retained > 0) {
        ImGui::BulletText(micro.accuracy >= 0.90 && round.microJitter <= 0.13 &&
                                 round.testScores[0] >= 55.0
                             ? "Steady micro-adjustments: %.0f%% first-shot hit rate, %.3f deg path deviation."
                             : "Micro-adjustments: %.0f%% first-shot hit rate, %.3f deg path deviation.",
                         micro.accuracy * 100.0, round.microJitter);
    }
    const auto jitter = std::find_if(history.rbegin(), history.rend(),
                                     [](const auto& entry) { return entry.highJitter; });
    if (jitter != history.rend()) {
        ImGui::TextColored(Warm, "High Jitter in round %d (%.3f deg RMS). Sens increases were locked.",
                           jitter->number, jitter->trackingJitter);
    }
}
}

double CentimetersPerTurn(int dpi, double sensitivity) {
    return 360.0 * 2.54 / (dpi * sensitivity * ValorantDegreesPerCount);
}

std::filesystem::path SaveCalibrationHistory(const AdaptiveSensTuner& tuner) {
    const auto* recommendation = tuner.Recommendation();
    if (!tuner.IsFinished() || !recommendation) {
        throw std::logic_error("Complete calibration before saving a result.");
    }
    const auto now = std::chrono::system_clock::now();
    const auto seconds = std::chrono::system_clock::to_time_t(now);
    const std::tm utc = *std::gmtime(&seconds);
    const auto directory = ApplicationDirectory() / "history";
    std::filesystem::create_directories(directory);
    std::ostringstream name;
    name.imbue(std::locale::classic());
    name << "calibration-" << std::put_time(&utc, "%Y-%m-%d")
         << '-' << now.time_since_epoch().count();
    const auto destination = directory / (name.str() + ".csv");
    const auto temporary = directory / (name.str() + ".tmp");
    std::ofstream file(temporary);
    file.imbue(std::locale::classic());
    file << std::fixed << std::setprecision(6)
         << "saved_utc,round,recommended,dpi,sensitivity,edpi,cm_per_360,throughput_bits_s,"
            "composite_score,bias,micro_score,flick_score,switch_score,tracking_score,"
            "micro_throughput,flick_throughput,switch_throughput,micro_accuracy_pct,"
            "flick_accuracy_pct,switch_accuracy_pct,micro_bias,flick_bias,switch_bias,"
            "micro_path_jitter_deg,tracking_accuracy_pct,tracking_jitter_deg,"
            "outliers_removed,high_jitter,increase_locked,backtracked,early_stop,candidate_mean_score\n";
    for (const auto& round : tuner.History()) {
        const int dpi = tuner.Dpi();
        file << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ") << ',' << round.number << ','
             << (round.sensitivity == recommendation->sensitivity) << ',' << dpi << ',' << round.sensitivity
             << ',' << dpi * round.sensitivity << ',' << CentimetersPerTurn(dpi, round.sensitivity)
             << ',' << round.throughput << ',' << round.score << ',' << round.bias;
        for (const auto score : round.testScores) file << ',' << score;
        for (const auto& aim : round.aim) file << ',' << aim.throughput;
        for (const auto& aim : round.aim) file << ',' << aim.accuracy * 100.0;
        for (const auto& aim : round.aim) file << ',' << aim.bias;
        file << ',' << round.microJitter << ',' << round.trackingAccuracy * 100.0 << ','
             << round.trackingJitter << ','
             << round.removedTrials << ',' << round.highJitter << ',' << round.increaseLocked
             << ',' << round.backtracked << ',' << round.earlyStop << ',';
        if (round.sensitivity == recommendation->sensitivity) {
            file << tuner.AverageScore(round.sensitivity);
        }
        file << '\n';
    }
    file.close();
    std::error_code error;
    if (file) {
        std::filesystem::rename(temporary, destination, error);
        if (!error) {
            return destination;
        }
    }
    std::filesystem::remove(temporary, error);
    throw std::runtime_error("Could not write the history CSV.");
}

void CalibrationReport::Reset() {
    status_.clear();
    savedPath_.clear();
    actionFailed_ = false;
}

void CalibrationReport::Draw(const AdaptiveSensTuner& tuner) {
    const auto* best = tuner.Recommendation();
    if (!best) {
        return;
    }
    const auto& history = tuner.History();
    const std::string sens = FormatSensitivity(best->sensitivity);
    ImGui::TextColored(Accent, "Recommended sensitivity");
    ImGui::PushFont(nullptr, 29.0f);
    ImGui::TextUnformatted(sens.c_str());
    ImGui::PopFont();
    ImGui::Text("%.0f eDPI   |   %.2f cm/360 deg   |   %d DPI   |   Mean score %.1f / 100",
                best->sensitivity * tuner.Dpi(), CentimetersPerTurn(tuner.Dpi(), best->sensitivity),
                tuner.Dpi(), tuner.AverageScore(best->sensitivity));
    ImGui::TextDisabled(history.back().earlyStop ? "Converged" : "Six rounds complete");

    std::array<float, AdaptiveSensTuner::MaxRounds> sensitivity{};
    std::array<float, AdaptiveSensTuner::MaxRounds> throughput{};
    for (std::size_t i = 0; i < history.size(); ++i) {
        sensitivity[i] = static_cast<float>(history[i].sensitivity);
        throughput[i] = static_cast<float>(history[i].throughput);
    }
    ImGui::SeparatorText("Round trends");
    const auto count = static_cast<int>(history.size());
    if (ImGui::BeginTable("Trends", 2, ImGuiTableFlags_SizingStretchSame)) {
        ImGui::TableNextColumn();
        ImGui::PushID("sens");
        PlotHistory("Sensitivity", sensitivity.data(), count, Accent);
        ImGui::PopID();
        ImGui::TableNextColumn();
        ImGui::PushID("tp");
        PlotHistory("Fitts's throughput (bits/s)", throughput.data(), count, Warm);
        ImGui::PopID();
        ImGui::EndTable();
    }

    ImGui::SeparatorText("Skill breakdown");
    ImGui::TextDisabled("First measured round at this sensitivity: %d", best->number);
    DrawSkillScores(*best);
    DrawFindings(*best, history);
    ImGui::Spacing();
    if (ImGui::Button("Copy Sens")) {
        SetClipboardText(sens.c_str());
        const char* copied = GetClipboardText();
        actionFailed_ = !copied || std::strcmp(copied, sens.c_str()) != 0;
        status_ = actionFailed_ ? "Could not update the clipboard." : "Sensitivity copied.";
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(!savedPath_.empty());
    if (ImGui::Button("Save Results")) {
        try {
            savedPath_ = SaveCalibrationHistory(tuner);
            status_.clear();
            actionFailed_ = false;
        } catch (const std::exception& error) {
            status_ = error.what();
            actionFailed_ = true;
        }
    }
    ImGui::EndDisabled();
    if (!status_.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, actionFailed_ ? Warm : Accent);
        ImGui::TextWrapped("%s", status_.c_str());
        ImGui::PopStyleColor();
    }
    if (!savedPath_.empty()) {
        ImGui::TextWrapped("Saved to history/%s", DisplayPath(savedPath_.filename()).c_str());
        ImGui::SetItemTooltip("%s", DisplayPath(savedPath_).c_str());
    }
}
