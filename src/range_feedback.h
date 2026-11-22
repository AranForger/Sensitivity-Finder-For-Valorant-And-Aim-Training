#pragma once

#include "customization.h"

#include <array>
#include <cstddef>
#include <optional>

enum class FeedbackCue { Shot, Hit, Countdown };

class RangeFeedback {
public:
    RangeFeedback();
    ~RangeFeedback();
    RangeFeedback(const RangeFeedback&) = delete;
    RangeFeedback& operator=(const RangeFeedback&) = delete;

    void Shot(double now, std::optional<Vector3> hit);
    void UpdateCountdown(double remaining);
    void SetAudio(const AudioPreferences& audio);
    void Preview(FeedbackCue cue);
    bool IsCueAvailable(FeedbackCue cue) const;
    void DrawImpacts(double now, Vector3 eye, Color impactColor) const;
    bool HasImpact(double now) const;

private:
    Sound CueSound(FeedbackCue cue) const;

    struct Impact {
        Vector3 point{};
        double started = -1.0;
    };

    static constexpr double ImpactSeconds = 0.080;
    std::array<Impact, 8> impacts_{};
    std::size_t nextImpact_ = 0;
    Sound click_{};
    Sound hit_{};
    Sound beep_{};
    int lastCountdown_ = 0;
    bool audioReady_ = false;
};
