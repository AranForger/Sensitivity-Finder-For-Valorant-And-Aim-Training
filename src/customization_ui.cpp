#include "customization_ui.h"

#include "range_feedback.h"

#include <imgui.h>

#include <cmath>

namespace {
ImU32 ImColor(Color color) {
    return IM_COL32(color.r, color.g, color.b, color.a);
}

void ColorPicker(const char* label, Color& color) {
    float rgb[]{color.r / 255.0f, color.g / 255.0f, color.b / 255.0f};
    if (ImGui::ColorEdit3(label, rgb, ImGuiColorEditFlags_Uint8 | ImGuiColorEditFlags_NoInputs)) {
        color = {static_cast<unsigned char>(std::lround(rgb[0] * 255.0f)),
                 static_cast<unsigned char>(std::lround(rgb[1] * 255.0f)),
                 static_cast<unsigned char>(std::lround(rgb[2] * 255.0f)), 255};
    }
}

void DrawPalettePreview(const RangePalette& palette) {
    ImGui::SeparatorText("Preview");
    const auto start = ImGui::GetCursorScreenPos();
    const float width = ImGui::GetContentRegionAvail().x;
    constexpr float height = 116.0f;
    const auto point = [&](float x, float y) {
        return ImVec2{start.x + x * width, start.y + y};
    };
    auto& draw = *ImGui::GetWindowDrawList();
    draw.AddRectFilled(point(0, 0), point(1, height), ImColor(palette.background));
    draw.AddQuadFilled(point(0, 0), point(0.10f, 15), point(0.10f, 79), point(0, height),
                       ImColor(palette.sideWalls));
    draw.AddQuadFilled(point(1, 0), point(0.90f, 15), point(0.90f, 79), point(1, height),
                       ImColor(palette.sideWalls));
    draw.AddQuadFilled(point(0.10f, 79), point(0.90f, 79), point(1, height), point(0, height),
                       ImColor(palette.floor));
    draw.AddLine(point(0, 99), point(1, 99), ImColor(palette.grid));
    draw.AddLine(point(0.28f, 79), point(0.18f, height), ImColor(palette.grid));
    draw.AddLine(point(0.72f, 79), point(0.82f, height), ImColor(palette.grid));
    draw.AddLine(point(0.10f, 42), point(0.90f, 42), ImColor(palette.headLine), 2.0f);
    draw.AddCircleFilled(point(0.24f, 42), 9.0f, ImColor(palette.target));
    draw.AddCircleFilled(point(0.40f, 42), 12.0f, ImColor(palette.anchor));
    const auto agent = [&](float x, Color head, Color body) {
        const float halfWidth = 9.0f / width;
        draw.AddRectFilled(point(x - halfWidth, 54), point(x + halfWidth, 83), ImColor(body), 9.0f);
        draw.AddCircleFilled(point(x, 42), 8.0f, ImColor(head));
    };
    agent(0.59f, palette.target, palette.trackingBody);
    agent(0.75f, palette.trackingOnTarget, palette.trackingOnTarget);
    draw.AddLine(point(0.82f, 35), point(0.86f, 49), ImColor(palette.impact), 2.0f);
    draw.AddLine(point(0.82f, 49), point(0.86f, 35), ImColor(palette.impact), 2.0f);
    ImGui::Dummy({width, height});
}
}

void DrawAppearanceSettings(RangePalette& palette) {
    if (ImGui::BeginTable("Palette", 2, ImGuiTableFlags_SizingStretchSame)) {
        ImGui::TableSetupColumn("Range");
        ImGui::TableSetupColumn("Targets and feedback");
        ImGui::TableHeadersRow();
        for (const auto& field : PaletteFields) {
            ImGui::TableNextColumn();
            ColorPicker(field.label, palette.*field.value);
        }
        ImGui::EndTable();
    }
    DrawPalettePreview(palette);
    if (ImGui::Button("Reset colors")) {
        palette = {};
    }
}

void DrawAudioSettings(AudioPreferences& audio, RangeFeedback& feedback) {
    if (ImGui::Checkbox("Mute all sounds", &audio.muted)) {
        feedback.SetAudio(audio);
    }
    ImGui::SetNextItemWidth(250.0f);
    if (ImGui::SliderInt("Master volume", &audio.master, 0, 100, "%d%%")) {
        feedback.SetAudio(audio);
    }
    ImGui::SeparatorText("Sound effects");
    const auto channel = [&](const char* label, int& volume, FeedbackCue cue) {
        ImGui::PushID(label);
        ImGui::SetNextItemWidth(250.0f);
        if (ImGui::SliderInt(label, &volume, 0, 100, "%d%%")) {
            feedback.SetAudio(audio);
        }
        ImGui::SameLine();
        ImGui::BeginDisabled(audio.muted || audio.master == 0 || volume == 0 ||
                             !feedback.IsCueAvailable(cue));
        if (ImGui::Button("Preview")) {
            feedback.Preview(cue);
        }
        ImGui::EndDisabled();
        ImGui::PopID();
    };
    channel("Shot", audio.shot, FeedbackCue::Shot);
    channel("Hit", audio.hit, FeedbackCue::Hit);
    channel("Countdown", audio.countdown, FeedbackCue::Countdown);
    if (!feedback.IsCueAvailable(FeedbackCue::Shot) && !feedback.IsCueAvailable(FeedbackCue::Hit) &&
        !feedback.IsCueAvailable(FeedbackCue::Countdown)) {
        ImGui::TextDisabled("Audio output unavailable");
    }
    if (ImGui::Button("Reset audio")) {
        audio = {};
        feedback.SetAudio(audio);
    }
}
