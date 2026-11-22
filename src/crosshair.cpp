#include "crosshair.h"

#include <imgui.h>

#include <array>
#include <cmath>

namespace {
constexpr const char* ColorNames[Crosshair::ColorCount]{"Cyan", "Green", "White", "Yellow", "Red"};
constexpr ImU32 Colors[Crosshair::ColorCount]{
    IM_COL32(0, 255, 255, 255), IM_COL32(0, 255, 80, 255), IM_COL32(255, 255, 255, 255),
    IM_COL32(255, 232, 0, 255), IM_COL32(255, 48, 55, 255)
};

struct Rect {
    ImVec2 min;
    ImVec2 max;
};
}

void Crosshair::Draw(ImDrawList& draw, ImVec2 center) const {
    const float x = std::floor(center.x);
    const float y = std::floor(center.y);
    const float breadth = static_cast<float>(thickness);
    const float half = std::floor(breadth * 0.5f);
    const float x0 = x - half;
    const float y0 = y - half;
    const float gap = static_cast<float>(offset);
    const float size = static_cast<float>(length);
    const float left = x - gap;
    const float right = x + gap;
    const float top = y - gap;
    const float bottom = y + gap;

    std::array<Rect, 5> bars{};
    std::size_t count = 0;
    if (length > 0) {
        bars[count++] = {{left - size, y0}, {left, y0 + breadth}};
        bars[count++] = {{right, y0}, {right + size, y0 + breadth}};
        bars[count++] = {{x0, top - size}, {x0 + breadth, top}};
        bars[count++] = {{x0, bottom}, {x0 + breadth, bottom + size}};
    }
    if (centerDot) {
        bars[count++] = {{x0, y0}, {x0 + breadth, y0 + breadth}};
    }
    if (outlines) {
        for (std::size_t i = 0; i < count; ++i) {
            const auto& bar = bars[i];
            draw.AddRectFilled({bar.min.x - 1.0f, bar.min.y - 1.0f},
                               {bar.max.x + 1.0f, bar.max.y + 1.0f}, IM_COL32(0, 0, 0, 255));
        }
    }
    for (std::size_t i = 0; i < count; ++i) {
        draw.AddRectFilled(bars[i].min, bars[i].max, Colors[color]);
    }
}

void Crosshair::DrawSettings() {
    if (ImGui::Button("Small dot")) {
        *this = {0, 0, 2, 0, true, true};
    }
    ImGui::SameLine();
    if (ImGui::Button("Small cross 1-4-2-0")) {
        *this = {1, 4, 2, 0, false, false};
    }
    ImGui::SameLine();
    if (ImGui::Button("Thin cross 1-3-1-2")) {
        *this = {2, 3, 1, 2, false, true};
    }

    ImGui::Spacing();
    ImGui::SetNextItemWidth(220.0f);
    ImGui::Combo("Color", &color, ColorNames, IM_ARRAYSIZE(ColorNames));
    ImGui::SetNextItemWidth(220.0f);
    ImGui::SliderInt("Inner line length", &length, 0, MaxLength, "%d px");
    ImGui::SetNextItemWidth(220.0f);
    ImGui::SliderInt("Thickness", &thickness, 1, MaxThickness, "%d px");
    ImGui::SetNextItemWidth(220.0f);
    ImGui::SliderInt("Offset", &offset, 0, MaxOffset, "%d px");
    ImGui::Checkbox("Center dot", &centerDot);
    ImGui::SameLine();
    ImGui::Checkbox("Black outlines", &outlines);

    ImGui::SeparatorText("Live preview");
    const auto start = ImGui::GetCursorScreenPos();
    const float width = ImGui::GetContentRegionAvail().x;
    constexpr float height = 90.0f;
    auto& draw = *ImGui::GetWindowDrawList();
    draw.AddRectFilled(start, {start.x + width, start.y + height * 0.5f}, IM_COL32(18, 26, 36, 255));
    draw.AddRectFilled({start.x, start.y + height * 0.5f}, {start.x + width, start.y + height},
                       IM_COL32(144, 157, 166, 255));
    Draw(draw, {start.x + width * 0.5f, start.y + height * 0.5f});
    ImGui::Dummy({width, height});
}
