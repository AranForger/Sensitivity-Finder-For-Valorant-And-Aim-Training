#pragma once

#include "adaptive_sens_tuner.h"

#include <filesystem>
#include <string>

double CentimetersPerTurn(int dpi, double sensitivity);
std::filesystem::path SaveCalibrationHistory(const AdaptiveSensTuner& tuner);

class CalibrationReport {
public:
    void Reset();
    void Draw(const AdaptiveSensTuner& tuner);

private:
    std::string status_;
    std::filesystem::path savedPath_;
    bool actionFailed_ = false;
};
