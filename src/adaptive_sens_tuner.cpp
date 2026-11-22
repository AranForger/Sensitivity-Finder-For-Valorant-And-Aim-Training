#include "adaptive_sens_tuner.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace {
constexpr std::array Weights{0.40, 0.30, 0.20, 0.10};
constexpr double AimWeight = Weights[0] + Weights[1] + Weights[2];
constexpr double ThroughputReference = 5.0;  // Bits/s at half of the normalization scale.
constexpr double MinimumJitterSeconds = 0.75;
constexpr double ProbeFraction = 0.06;
constexpr double ScoreStability = 3.0;

double Median(std::vector<double> values) {
    std::sort(values.begin(), values.end());
    const auto middle = values.size() / 2;
    return (values[(values.size() - 1) / 2] + values[middle]) * 0.5;
}
}

AimStatistics AnalyzeAim(std::span<const AimTrial> trials) {
    AimStatistics stats;
    std::unordered_map<int, const AimTrial*> firstClicks;
    std::vector<const AimTrial*> completed;
    std::vector<double> times;
    for (const auto& shot : trials) {
        if (std::isfinite(shot.mt) && shot.mt > 0.0) {
            firstClicks.try_emplace(shot.targetId, &shot);
            if (shot.hit) {
                completed.push_back(&shot);
                times.push_back(shot.mt);
            }
        }
    }
    if (times.empty()) return stats;

    const double median = Median(times);
    for (auto& time : times) {
        time = std::abs(time - median);
    }
    const double limit = std::max(2.5 * Median(times), 0.05 * median);
    std::size_t firstHits = 0;
    double meanProjection = 0.0;
    double meanError = 0.0;
    double errorSquares = 0.0;
    double directionalError = 0.0;
    std::vector<double> widths;
    for (const auto* hit : completed) {
        if (std::abs(hit->mt - median) > limit) {
            ++stats.removed;
            continue;
        }
        const double count = static_cast<double>(++stats.retained);
        const auto& first = *firstClicks.at(hit->targetId);
        firstHits += first.hit;
        if (first.width > 0.0) {
            const double endpoint = (first.projection - first.ideal) / first.width;
            const double outside = std::clamp((std::abs(endpoint) - 0.10) / 0.90, 0.0, 1.0);
            const double peak = (first.maxProjection - first.ideal) / first.width;
            const double correction = std::abs(endpoint) <= 0.5 ?
                0.25 * std::clamp(peak - 0.5, 0.0, 1.0) : 0.0;
            directionalError += std::clamp(std::copysign(outside, endpoint) + correction, -1.0, 1.0);
        }
        if (hit->width > 0.0) widths.push_back(hit->width);
        stats.meanMt += (hit->mt - stats.meanMt) / count;
        meanProjection += (hit->projection - meanProjection) / count;
        const double error = hit->projection - hit->ideal;
        const double delta = error - meanError;
        meanError += delta / count;
        errorSquares += delta * (error - meanError);
    }
    if (stats.retained == 0) return stats;
    const double retained = static_cast<double>(stats.retained);
    stats.accuracy = static_cast<double>(firstHits) / retained;
    stats.bias = std::tanh(1.5 * directionalError / retained);
    if (stats.retained >= 2) {
        const double floor = widths.empty() ? 0.0 : Median(widths) * 0.25;
        stats.effectiveWidth = std::max(floor, 4.133 *
            std::sqrt(std::max(0.0, errorSquares) / (retained - 1.0)));
        if (stats.effectiveWidth > 0.0 && meanProjection > 0.0) {
            stats.throughput = std::log1p(meanProjection / stats.effectiveWidth) /
                               (std::log(2.0) * stats.meanMt);
        }
    }
    return stats;
}

double AdaptiveSensTuner::ClampSensitivity(int dpi, double sensitivity) {
    const int validDpi = std::max(dpi, 1);
    if (!std::isfinite(sensitivity) || sensitivity <= 0.0) {
        sensitivity = SuggestedEdpi / validDpi;
    }
    return std::clamp(sensitivity, MinEdpi / validDpi, MaxEdpi / validDpi);
}

void AdaptiveSensTuner::Reset(int dpi, double startingSensitivity) {
    dpi_ = std::max(dpi, 1);
    sensitivity_ = ClampSensitivity(dpi_, startingSensitivity);
    startingSensitivity_ = sensitivity_;
    probeDirection_ = 1.0;
    upperSensitivity_ = MaxEdpi / dpi_;
    increaseLocked_ = false;
    history_.clear();
}

bool AdaptiveSensTuner::IsFinished() const {
    return !history_.empty() &&
        (history_.back().earlyStop || history_.back().number >= MaxRounds);
}

void AdaptiveSensTuner::FinishRound(const CalibrationMeasurements& measurements) {
    if (IsFinished()) {
        return;
    }

    CalibrationRound round;
    round.number = RoundNumber();
    round.sensitivity = sensitivity_;
    round.aim = measurements.aim;
    round.microJitter = measurements.microJitter;
    round.trackingAccuracy = measurements.trackingAccuracy;
    double biasWeight = 0.0;
    for (std::size_t i = 0; i < measurements.aim.size(); ++i) {
        const auto& aim = measurements.aim[i];
        round.testScores[i] = 100.0 * aim.accuracy *
            (aim.throughput / (aim.throughput + ThroughputReference));
        round.throughput += Weights[i] * aim.throughput / AimWeight;
        if (aim.retained > 0) {
            round.bias += Weights[i] * aim.bias;
            biasWeight += Weights[i];
        }
        round.removedTrials += aim.removed;
    }
    if (biasWeight > 0.0) {
        round.bias /= biasWeight;
    }
    round.trackingJitter = measurements.perpendicularJitter;
    const bool excessiveJitter = measurements.jitterSeconds >= MinimumJitterSeconds &&
                                 round.trackingJitter > JitterThreshold;
    round.highJitter = excessiveJitter && sensitivity_ >= HighEdpi / dpi_;
    const double jitterFactor = excessiveJitter ? JitterThreshold / round.trackingJitter : 1.0;
    round.testScores[3] = 100.0 * measurements.trackingAccuracy * jitterFactor;
    for (std::size_t i = 0; i < round.testScores.size(); ++i) {
        round.score += Weights[i] * round.testScores[i];
    }

    increaseLocked_ = increaseLocked_ || round.highJitter;
    round.increaseLocked = increaseLocked_;
    if (increaseLocked_) {
        upperSensitivity_ = std::min(upperSensitivity_, sensitivity_);
    }
    history_.push_back(round);
    auto ranked = RankCandidates();
    const auto& leader = ranked.front();
    const auto competitor = ranked.size() > 1 ? &ranked[1] : nullptr;
    const auto limit = [this](double sens) {
        return std::min(ClampSensitivity(dpi_, sens), upperSensitivity_);
    };
    double next = leader.representative->sensitivity;
    if (round.number == 1) {
        probeDirection_ = round.bias > 0.08 ? -1.0 : 1.0;
        next = limit(startingSensitivity_ * std::exp(probeDirection_ * ProbeFraction));
        if (next == sensitivity_) {
            probeDirection_ = -probeDirection_;
            next = limit(startingSensitivity_ * std::exp(probeDirection_ * ProbeFraction));
        }
    } else if (round.number == 2) {
        next = limit(startingSensitivity_ * std::exp(-probeDirection_ * ProbeFraction));
    } else if (round.number == 4 && competitor) {
        // Verify a distinct contender after retesting the first leader.
        next = leader.representative->sensitivity == sensitivity_ ?
            competitor->representative->sensitivity : leader.representative->sensitivity;
    }
    auto& saved = history_.back();
    saved.nextSensitivity = limit(next);
    saved.backtracked = round.number == 5 && saved.nextSensitivity != sensitivity_ &&
        round.throughput < 0.90 * leader.representative->throughput;
    saved.earlyStop = round.number == 5 && leader.rounds >= 2 &&
        leader.spread <= ScoreStability && std::abs(leader.bias) < 0.08 &&
        (!competitor || (competitor->rounds >= 2 && leader.score - competitor->score >= 2.0));
    sensitivity_ = saved.nextSensitivity;
}

std::vector<AdaptiveSensTuner::Candidate> AdaptiveSensTuner::RankCandidates() const {
    std::vector<Candidate> candidates;
    for (const auto& round : history_) {
        if (round.sensitivity > upperSensitivity_) continue;
        auto existing = std::find_if(candidates.begin(), candidates.end(), [&](const Candidate& entry) {
            return entry.representative->sensitivity == round.sensitivity;
        });
        if (existing == candidates.end()) {
            candidates.push_back({&round, round.score, round.bias, 0.0, 1,
                                  round.score, round.score});
        } else {
            ++existing->rounds;
            existing->score += (round.score - existing->score) / existing->rounds;
            existing->bias += (round.bias - existing->bias) / existing->rounds;
            existing->lowest = std::min(existing->lowest, round.score);
            existing->highest = std::max(existing->highest, round.score);
            existing->spread = existing->highest - existing->lowest;
        }
    }
    std::stable_sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
        const double aRank = a.score - (a.rounds == 1 ? 2.0 : 0.0);
        const double bRank = b.score - (b.rounds == 1 ? 2.0 : 0.0);
        return aRank > bRank;
    });
    return candidates;
}

const CalibrationRound* AdaptiveSensTuner::Recommendation() const {
    const auto candidates = RankCandidates();
    if (candidates.empty()) return nullptr;
    const auto confirmed = std::find_if(candidates.begin(), candidates.end(),
        [](const Candidate& candidate) { return candidate.rounds >= 2; });
    return (confirmed == candidates.end() ? candidates.front() : *confirmed).representative;
}

double AdaptiveSensTuner::AverageScore(double sensitivity) const {
    const auto candidates = RankCandidates();
    for (const auto& candidate : candidates) {
        if (candidate.representative->sensitivity == sensitivity) return candidate.score;
    }
    return 0.0;
}
