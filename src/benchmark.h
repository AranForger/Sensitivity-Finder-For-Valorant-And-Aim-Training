#pragma once

#include "adaptive_sens_tuner.h"
#include "customization.h"
#include "raw_mouse.h"

#include <raylib.h>

#include <array>
#include <cstdint>
#include <fstream>
#include <optional>
#include <random>
#include <string>
#include <vector>

inline constexpr float EyeHeight = 160.0f;
inline constexpr float HeadHeight = 165.0f;
inline constexpr Vector3 EyePosition{0.0f, EyeHeight, 0.0f};

Vector3 AimDirection(LookAngles look);

enum class Test { Micro, Flick, Switch, Tracking };
enum class Mode { Full, Condensed };

inline constexpr std::array BenchmarkOrder{Test::Micro, Test::Flick, Test::Switch, Test::Tracking};

const char* TestName(Test test);
const char* ModeName(Mode mode);

struct BenchmarkResult {
    AimStatistics aim;
    double jitter = 0.0;
    double meanSwitchDelay = 0.0;
    double pathEfficiency = 0.0;
    double timeOnTarget = 0.0;
    double meanReaction = 0.0;
    double perpendicularJitter = 0.0;
    double jitterSeconds = 0.0;
    int reactions = 0;
    int correctedOvershoots = 0;
};

class Benchmark {
public:
    void Start(Test test, Mode mode, double sensitivity, double now,
               std::optional<std::uint32_t> seed = std::nullopt);
    void Stop();
    void SetPaused(bool paused, double now);
    void Advance(double now);
    std::optional<Vector3> Observe(MouseSample sample);
    void Draw(const RangePalette& palette) const;

    bool IsRunning() const { return active_ && !finished_; }
    bool IsFinished() const { return finished_; }
    Test Kind() const { return test_; }
    Mode Length() const { return mode_; }
    int Completed() const { return completed_; }
    int Goal() const { return goal_; }
    bool AwaitingAnchor() const { return targetCount_ == 1 && targets_[0].anchor; }
    bool IsReactive() const { return elapsed_ >= phaseSeconds_; }
    double SecondsLeft() const { return 2.0 * phaseSeconds_ - elapsed_; }
    const BenchmarkResult& Result() const { return result_; }
    const std::string& TelemetryPath() const { return path_; }
    bool TelemetryFailed() const { return logFailed_; }

private:
    struct Motion {
        LookAngles start;
        LookAngles target;
        double started = 0.0;
        double ideal = 0.0;
        double maxProjection = 0.0;
        double actual = 0.0;
        double perpendicularSquared = 0.0;
        int samples = 0;

        double Project(LookAngles look) const;
        void Observe(LookAngles previous, LookAngles look);
    };

    struct Target {
        Vector3 center;
        Motion motion;
        int id = 0;
        bool anchor = false;
    };

    struct HitMetrics {
        double mt;
        double ideal;
        double actual;
        double jitter;
        bool correctedOvershoot;
    };

    double Uniform(double low, double high);
    int RandomSign();
    void AddTarget(Vector3 center, bool anchor = false);
    void SpawnMicro();
    void SpawnFlick();
    void SpawnCluster();
    void SpawnAnchor();
    std::optional<Vector3> ClickTarget();
    void BeginTracking();
    void AdvanceTracking(double end);
    void UpdateTrackingMetrics(double dt);
    Vector3 TrackingHead() const;
    std::optional<Vector3> TrackingContact() const;
    std::optional<Vector3> ObserveTracking(LookAngles previous, bool clicked);
    void Record(const char* event, int id, bool hit, const Motion& motion,
                const LookAngles* click, double width);
    void Finish(bool completed);

    std::mt19937 random_{std::random_device{}()};
    std::array<Target, 3> targets_{};
    std::vector<HitMetrics> hits_;
    std::vector<AimTrial> shots_;
    std::vector<LookAngles> trackingPath_;
    std::ofstream log_;
    std::string path_;
    BenchmarkResult result_;
    Test test_ = Test::Micro;
    Mode mode_ = Mode::Full;
    LookAngles look_{};
    double elapsed_ = 0.0;
    double lastTime_ = 0.0;
    double sensitivity_ = 0.0;
    double microDistance_ = 2000.0;
    int targetCount_ = 0;
    int nextId_ = 1;
    int completed_ = 0;
    int goal_ = 0;
    bool active_ = false;
    bool paused_ = false;
    bool finished_ = false;
    bool logFailed_ = false;

    double phaseSeconds_ = 10.0;
    double trackingX_ = 0.0;
    double velocity_ = 0.0;
    int direction_ = 1;
    double nextReverse_ = 0.0;
    double nextJump_ = 0.0;
    double jumpStart_ = -1.0;
    double jumpHeight_ = 0.0;
    double reactionStart_ = -1.0;
    double reactionMotion_ = 0.0;
    double reactionSum_ = 0.0;
    int reactionCount_ = 0;
    bool braking_ = false;
    double onTarget_ = 0.0;
    LookAngles filteredError_{};
    double jitterIntegral_ = 0.0;
    double perpendicularBaseline_ = 0.0;
    double perpendicularBand_ = 0.0;
    double perpendicularSquares_ = 0.0;
    double perpendicularSeconds_ = 0.0;
    LookAngles trackingStart_{};
    double trackingStarted_ = 0.0;
};
