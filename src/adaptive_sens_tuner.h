#pragma once

#include <array>
#include <cstddef>
#include <span>
#include <vector>

struct AimTrial {
    double mt;
    double projection;
    double maxProjection;
    double ideal;
    double width;
    bool hit;
    int targetId;
};

struct AimStatistics {
    double meanMt = 0.0;
    double effectiveWidth = 0.0;
    double throughput = 0.0;
    double accuracy = 0.0;
    double bias = 0.0;
    std::size_t retained = 0;
    std::size_t removed = 0;
};

AimStatistics AnalyzeAim(std::span<const AimTrial> trials);

struct CalibrationMeasurements {
    std::array<AimStatistics, 3> aim{};  // Micro, Flick, Switch
    double trackingAccuracy = 0.0;
    double microJitter = 0.0;
    double perpendicularJitter = 0.0;
    double jitterSeconds = 0.0;
};

struct CalibrationRound {
    int number = 0;
    double sensitivity = 0.0;
    double nextSensitivity = 0.0;
    double throughput = 0.0;
    double bias = 0.0;
    double score = 0.0;
    double microJitter = 0.0;
    double trackingAccuracy = 0.0;
    double trackingJitter = 0.0;
    std::array<AimStatistics, 3> aim{};
    std::array<double, 4> testScores{};
    std::size_t removedTrials = 0;
    bool highJitter = false;
    bool increaseLocked = false;
    bool backtracked = false;
    bool earlyStop = false;
};

class AdaptiveSensTuner {
public:
    static constexpr int MaxRounds = 6;
    static constexpr double SuggestedEdpi = 280.0;
    static constexpr double MinEdpi = 150.0;
    static constexpr double MaxEdpi = 600.0;
    static constexpr double JitterThreshold = 0.10;  // Degrees RMS in the range.
    static constexpr double HighEdpi = 350.0;

    static double ClampSensitivity(int dpi, double sensitivity);
    void Reset(int dpi, double startingSensitivity);
    void FinishRound(const CalibrationMeasurements& measurements);
    bool IsFinished() const;
    double CurrentSensitivity() const { return sensitivity_; }
    int Dpi() const { return dpi_; }
    int RoundNumber() const { return static_cast<int>(history_.size()) + 1; }
    const std::vector<CalibrationRound>& History() const { return history_; }
    const CalibrationRound* Recommendation() const;
    double AverageScore(double sensitivity) const;

private:
    struct Candidate {
        const CalibrationRound* representative = nullptr;
        double score = 0.0;
        double bias = 0.0;
        double spread = 0.0;
        int rounds = 0;
        double lowest = 0.0;
        double highest = 0.0;
    };

    std::vector<Candidate> RankCandidates() const;

    int dpi_ = 800;
    double sensitivity_ = SuggestedEdpi / dpi_;
    double startingSensitivity_ = sensitivity_;
    double probeDirection_ = 1.0;
    double upperSensitivity_ = MaxEdpi / dpi_;
    bool increaseLocked_ = false;
    std::vector<CalibrationRound> history_;
};
