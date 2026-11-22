#include "auto_calibration.h"

#include <algorithm>

void AutoCalibration::Start(Benchmark& benchmark, int dpi, double sensitivity, bool blind, double now) {
    benchmark.Stop();
    blind_ = blind;
    if (blind_) {
        constexpr double spread = 35.0;
        std::mt19937 random(std::random_device{}());
        const double edpi = std::uniform_real_distribution<double>(
            AdaptiveSensTuner::SuggestedEdpi - spread,
            AdaptiveSensTuner::SuggestedEdpi + spread)(random);
        sensitivity = edpi / std::max(dpi, 1);
    }
    tuner_.Reset(dpi, sensitivity);
    measurements_ = {};
    testIndex_ = 0;
    scenarioSeed_ = std::random_device{}();
    paused_ = false;
    state_ = State::Resting;
    restUntil_ = now + RoundCountdownSeconds;
}

void AutoCalibration::Cancel(Benchmark& benchmark) {
    if (IsActive()) {
        benchmark.Stop();
    }
    state_ = State::Idle;
}

void AutoCalibration::StartTest(Benchmark& benchmark, double now) {
    benchmark.Start(CurrentTest(), Mode::Condensed, tuner_.CurrentSensitivity(), now,
                    scenarioSeed_ + static_cast<std::uint32_t>(testIndex_));
    state_ = State::Testing;
}

bool AutoCalibration::Advance(Benchmark& benchmark, double now) {
    if (paused_) {
        return false;
    }
    if (state_ == State::Resting && now >= restUntil_) {
        StartTest(benchmark, now);
        return true;
    }
    if (state_ != State::Testing || !benchmark.IsFinished()) {
        return false;
    }

    const auto& result = benchmark.Result();
    if (CurrentTest() == Test::Tracking) {
        measurements_.trackingAccuracy = result.timeOnTarget;
        measurements_.perpendicularJitter = result.perpendicularJitter;
        measurements_.jitterSeconds = result.jitterSeconds;
        tuner_.FinishRound(measurements_);
        measurements_ = {};
        testIndex_ = 0;
    } else {
        if (CurrentTest() == Test::Micro) {
            measurements_.microJitter = result.jitter;
        }
        measurements_.aim[testIndex_++] = result.aim;
    }
    benchmark.Stop();
    state_ = tuner_.IsFinished() ? State::Complete : State::Resting;
    restUntil_ = now + (IsRoundCountdown() ? RoundCountdownSeconds : RestSeconds);
    return false;
}

void AutoCalibration::SetPaused(Benchmark& benchmark, bool paused, double now) {
    if (!IsActive() || paused == paused_) {
        return;
    }
    paused_ = paused;
    if (state_ == State::Testing) {
        benchmark.SetPaused(paused, now);
    } else if (!paused) {
        restUntil_ += now - pausedAt_;
    }
    if (paused) {
        pausedAt_ = now;
    }
}

double AutoCalibration::RestRemaining(double now) const {
    return std::max(0.0, restUntil_ - (paused_ ? pausedAt_ : now));
}
