#include "range_feedback.h"

#include <raymath.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <vector>

namespace {
constexpr unsigned SampleRate = 44100;
constexpr double Tau = 2.0 * std::numbers::pi;

template <typename Sample>
Sound MakeSound(double seconds, Sample sample) {
    std::vector<std::int16_t> pcm(static_cast<std::size_t>(SampleRate * seconds));
    for (std::size_t i = 0; i < pcm.size(); ++i) {
        const double time = static_cast<double>(i) / SampleRate;
        const double fade = std::min({1.0, time / 0.001, (seconds - time) / 0.006});
        const double value = fade * sample(time);
        pcm[i] = static_cast<std::int16_t>(std::clamp(value, -1.0, 1.0) * 32767.0);
    }
    Wave wave{static_cast<unsigned>(pcm.size()), SampleRate, 16, 1, pcm.data()};
    return LoadSoundFromWave(wave);
}
}

RangeFeedback::RangeFeedback() {
    InitAudioDevice();
    audioReady_ = IsAudioDeviceReady();
    if (!audioReady_) {
        return;
    }
    click_ = MakeSound(0.035, [noise = std::uint32_t{8131}](double t) mutable {
        noise = noise * 1664525u + 1013904223u;
        const double hiss = (noise >> 16) / 32767.5 - 1.0;
        return std::exp(-t * 135.0) * (0.19 * hiss + 0.13 * std::sin(Tau * 220.0 * t));
    });
    hit_ = MakeSound(0.090, [](double t) {
        return 0.24 * std::exp(-t * 40.0) *
            (std::sin(Tau * 1260.0 * t) + 0.45 * std::sin(Tau * 1890.0 * t));
    });
    beep_ = MakeSound(0.085, [](double t) {
        return 0.16 * std::exp(-t * 19.0) * std::sin(Tau * 540.0 * t);
    });
}

RangeFeedback::~RangeFeedback() {
    if (audioReady_) {
        for (Sound sound : {beep_, hit_, click_}) {
            if (IsSoundValid(sound)) UnloadSound(sound);
        }
        CloseAudioDevice();
    }
}

Sound RangeFeedback::CueSound(FeedbackCue cue) const {
    switch (cue) {
        case FeedbackCue::Shot: return click_;
        case FeedbackCue::Hit: return hit_;
        case FeedbackCue::Countdown: return beep_;
    }
    return {};
}

bool RangeFeedback::IsCueAvailable(FeedbackCue cue) const {
    return IsSoundValid(CueSound(cue));
}

void RangeFeedback::SetAudio(const AudioPreferences& audio) {
    const float master = audio.muted ? 0.0f : audio.master / 100.0f;
    if (IsSoundValid(click_)) SetSoundVolume(click_, master * audio.shot / 100.0f);
    if (IsSoundValid(hit_)) SetSoundVolume(hit_, master * audio.hit / 100.0f);
    if (IsSoundValid(beep_)) SetSoundVolume(beep_, master * audio.countdown / 100.0f);
}

void RangeFeedback::Preview(FeedbackCue cue) {
    const auto sound = CueSound(cue);
    if (IsSoundValid(sound)) {
        if (cue == FeedbackCue::Countdown) SetSoundPitch(sound, 1.0f);
        PlaySound(sound);
    }
}

void RangeFeedback::Shot(double now, std::optional<Vector3> hit) {
    if (IsSoundValid(click_)) PlaySound(click_);
    if (hit) {
        if (IsSoundValid(hit_)) PlaySound(hit_);
        impacts_[nextImpact_] = {*hit, now};
        nextImpact_ = (nextImpact_ + 1) % impacts_.size();
    }
}

void RangeFeedback::UpdateCountdown(double remaining) {
    const int cue = static_cast<int>(std::ceil(remaining));
    if (cue > 0 && cue != lastCountdown_ && IsSoundValid(beep_)) {
        SetSoundPitch(beep_, cue == 1 ? 1.26f : 1.0f);
        PlaySound(beep_);
    }
    lastCountdown_ = cue;
}

bool RangeFeedback::HasImpact(double now) const {
    return std::any_of(impacts_.begin(), impacts_.end(), [now](const Impact& hit) {
        return hit.started >= 0.0 && now - hit.started < ImpactSeconds;
    });
}

void RangeFeedback::DrawImpacts(double now, Vector3 eye, Color impactColor) const {
    using namespace std::numbers;
    for (const auto& hit : impacts_) {
        const auto age = now - hit.started;
        if (hit.started < 0.0 || age >= ImpactSeconds) {
            continue;
        }
        const float progress = static_cast<float>(age / ImpactSeconds);
        const auto forward = Vector3Normalize(Vector3Subtract(hit.point, eye));
        const auto right = Vector3Normalize(Vector3CrossProduct(forward, {0.0f, 1.0f, 0.0f}));
        const auto up = Vector3CrossProduct(right, forward);
        const auto origin = Vector3Subtract(hit.point, Vector3Scale(forward, 3.0f));
        const auto color = Fade(impactColor, 1.0f - progress);
        DrawSphere(origin, 4.0f * (1.0f - progress), color);
        for (int i = 0; i < 6; ++i) {
            const float angle = pi_v<float> * (static_cast<float>(i) / 3.0f + 0.14f);
            const auto vector = Vector3Add(Vector3Scale(right, std::cos(angle)),
                                           Vector3Scale(up, std::sin(angle)));
            DrawLine3D(Vector3Add(origin, Vector3Scale(vector, 5.0f + 8.0f * progress)),
                       Vector3Add(origin, Vector3Scale(vector, 17.0f + 24.0f * progress)), color);
        }
    }
}
