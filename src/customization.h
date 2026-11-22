#pragma once

#include <raylib.h>

struct RangePalette {
    Color background{37, 48, 58, 255};
    Color floor{24, 31, 40, 255};
    Color sideWalls{29, 39, 49, 255};
    Color grid{48, 61, 73, 255};
    Color headLine{89, 233, 188, 255};
    Color target{255, 115, 112, 255};
    Color anchor{252, 202, 105, 255};
    Color trackingBody{204, 83, 94, 255};
    Color trackingOnTarget{50, 220, 80, 255};
    Color impact{255, 213, 101, 255};
};

struct PaletteField {
    const char* key;
    const char* label;
    Color RangePalette::*value;
};

inline constexpr PaletteField PaletteFields[]{
    {"background", "Background", &RangePalette::background},
    {"target", "Targets / tracking head", &RangePalette::target},
    {"floor", "Floor", &RangePalette::floor},
    {"anchor", "Center anchor", &RangePalette::anchor},
    {"side_walls", "Side walls", &RangePalette::sideWalls},
    {"tracking_body", "Tracking body", &RangePalette::trackingBody},
    {"grid", "Grid and pillars", &RangePalette::grid},
    {"tracking_on_target", "Tracking on target", &RangePalette::trackingOnTarget},
    {"head_line", "Head-level line", &RangePalette::headLine},
    {"impact", "Hit spark", &RangePalette::impact},
};

struct AudioPreferences {
    int master = 100;
    int shot = 100;
    int hit = 100;
    int countdown = 100;
    bool muted = false;
};
