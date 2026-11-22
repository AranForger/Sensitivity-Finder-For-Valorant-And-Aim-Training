#pragma once

struct ImDrawList;
struct ImVec2;

struct Crosshair {
    static constexpr int ColorCount = 5;
    static constexpr int MaxLength = 12;
    static constexpr int MaxThickness = 6;
    static constexpr int MaxOffset = 12;

    int color = 2;
    int length = 5;
    int thickness = 1;
    int offset = 3;
    bool centerDot = false;
    bool outlines = false;

    void Draw(ImDrawList& draw, ImVec2 center) const;
    void DrawSettings();
};
