#pragma once

#include "benchmark.h"

class AutoCalibration {
public:
    enum class State { Idle, Testing, Resting, Complete };
    static constexpr double RestSeconds = 2.0;
    static constexpr double RoundCountdownSeconds = 3.0;

    void Start(Benchmark& benchmark, int dpi, double sensitivity, bool blind, double now);
    void Cancel(Benchmark& benchmark);
    bool Advance(Benchmark& benchmark, double now);
    void SetPaused(Benchmark& benchmark, bool paused, double now);

    State Status() const { return state_; }
    bool IsActive() const { return state_ == State::Testing || state_ == State::Resting; }
    bool IsRoundCountdown() const { return state_ == State::Resting && testIndex_ == 0; }
    bool IsBlind() const { return blind_; }
    Test CurrentTest() const { return BenchmarkOrder[testIndex_]; }
    std::size_t TestNumber() const { return testIndex_ + 1; }
    double RestRemaining(double now) const;
    const AdaptiveSensTuner& Tuner() const { return tuner_; }

private:
    void StartTest(Benchmark& benchmark, double now);

    AdaptiveSensTuner tuner_;
    CalibrationMeasurements measurements_;
    State state_ = State::Idle;
    std::size_t testIndex_ = 0;
    std::uint32_t scenarioSeed_ = 0;
    bool paused_ = false;
    bool blind_ = false;
    double pausedAt_ = 0.0;
    double restUntil_ = 0.0;
};
