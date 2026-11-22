#include "benchmark.h"
#include "settings.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <locale>
#include <numbers>

namespace {
constexpr double Radians = std::numbers::pi / 180.0;
constexpr double Distance = 2000.0;
constexpr float HeadRadius = 11.0f;
constexpr float TrackingHeadRadius = 15.0f;
constexpr float FlickRadius = 24.0f;

float ClickTargetRadius(Test test) {
    return test == Test::Flick ? FlickRadius : HeadRadius;
}
constexpr float BodyRadius = 22.0f;
constexpr float BodyHeight = 140.0f;
constexpr double StrafeSpeed = 540.0;
constexpr double StrafeAcceleration = 3000.0;
const double SmoothLimit = Distance * std::tan(15.0 * Radians);

LookAngles Difference(LookAngles to, LookAngles from) {
    return {std::remainder(to.yaw - from.yaw, 360.0), to.pitch - from.pitch};
}

LookAngles AimAngles(Vector3 position) {
    return {std::atan2(-position.x, -position.z) / Radians,
            std::atan2(position.y - EyeHeight, std::hypot(position.x, position.z)) / Radians};
}

Vector3 PointAt(LookAngles angle, double distance) {
    const auto direction = AimDirection(angle);
    return {static_cast<float>(distance * direction.x),
            EyeHeight + static_cast<float>(distance * direction.y),
            static_cast<float>(distance * direction.z)};
}

double AngularWidth(Vector3 center, float radius) {
    return 2.0 * std::asin(radius / std::hypot(center.x, center.y - EyeHeight, center.z)) /
        Radians;
}

bool HitsBody(Ray ray, float x, float baseY) {
    const Vector3 low{x, baseY + BodyRadius, -static_cast<float>(Distance)};
    const Vector3 high{x, baseY + BodyHeight - BodyRadius, low.z};
    if (GetRayCollisionSphere(ray, low, BodyRadius).hit ||
        GetRayCollisionSphere(ray, high, BodyRadius).hit) {
        return true;
    }

    // Intersect the finite cylinder joining the two spherical cap centers.
    const double dx = ray.position.x - low.x;
    const double dz = ray.position.z - low.z;
    const double a = ray.direction.x * ray.direction.x + ray.direction.z * ray.direction.z;
    const double b = dx * ray.direction.x + dz * ray.direction.z;
    const double c = dx * dx + dz * dz - BodyRadius * BodyRadius;
    const double discriminant = b * b - a * c;
    if (a == 0.0 || discriminant < 0.0) {
        return false;
    }
    const double root = std::sqrt(discriminant);
    for (const double time : {(-b - root) / a, (-b + root) / a}) {
        const double y = ray.position.y + time * ray.direction.y;
        if (time >= 0.0 && y >= low.y && y <= high.y) {
            return true;
        }
    }
    return false;
}
}

Vector3 AimDirection(LookAngles look) {
    const double yaw = look.yaw * Radians;
    const double pitch = look.pitch * Radians;
    const double cosPitch = std::cos(pitch);
    return {static_cast<float>(-std::sin(yaw) * cosPitch),
            static_cast<float>(std::sin(pitch)),
            static_cast<float>(-std::cos(yaw) * cosPitch)};
}

const char* TestName(Test test) {
    switch (test) {
        case Test::Micro: return "Micro-Adjustment";
        case Test::Flick: return "Medium Flicking";
        case Test::Switch: return "Target Switching";
        case Test::Tracking: return "Smooth & Reactive Tracking";
    }
    return "";
}

const char* ModeName(Mode mode) {
    return mode == Mode::Full ? "Full" : "Condensed";
}

double Benchmark::Motion::Project(LookAngles look) const {
    if (ideal == 0.0) {
        return 0.0;
    }
    const auto axis = Difference(target, start);
    const auto displacement = Difference(look, start);
    return (displacement.yaw * axis.yaw + displacement.pitch * axis.pitch) / ideal;
}

void Benchmark::Motion::Observe(LookAngles previous, LookAngles look) {
    maxProjection = std::max(maxProjection, Project(look));
    const auto step = Difference(look, previous);
    actual += std::hypot(step.yaw, step.pitch);
    if (ideal > 0.0 && (step.yaw != 0.0 || step.pitch != 0.0)) {
        const auto axis = Difference(target, start);
        const auto displacement = Difference(look, start);
        const double perpendicular =
            (displacement.yaw * axis.pitch - displacement.pitch * axis.yaw) / ideal;
        perpendicularSquared += perpendicular * perpendicular;
        ++samples;
    }
}

void Benchmark::Start(Test test, Mode mode, double sensitivity, double now,
                      std::optional<std::uint32_t> seed) {
    Stop();
    if (seed) random_.seed(*seed);
    result_ = {};
    test_ = test;
    mode_ = mode;
    sensitivity_ = sensitivity;
    look_ = {};
    elapsed_ = 0.0;
    lastTime_ = now;
    targetCount_ = 0;
    nextId_ = 1;
    completed_ = 0;
    active_ = true;
    paused_ = false;
    finished_ = false;
    hits_.clear();
    shots_.clear();
    logFailed_ = false;

    std::error_code error;
    const auto directory = ApplicationDirectory() / "telemetry";
    std::filesystem::create_directories(directory, error);
    const auto tick = std::chrono::system_clock::now().time_since_epoch().count();
    const auto file = directory / ("aim-" + std::to_string(tick) + ".csv");
    path_ = DisplayPath(file);
    log_.clear();
    if (!error) {
        log_.open(file);
    }
    logFailed_ = !log_.is_open();
    if (!logFailed_) {
        log_.imbue(std::locale::classic());
        log_ << std::fixed << std::setprecision(6)
             << "test,mode,sensitivity,event,target_id,hit,start_yaw_deg,start_pitch_deg,"
                "target_yaw_deg,target_pitch_deg,click_yaw_deg,click_pitch_deg,mt_s,"
                "max_proj_deg,click_proj_deg,angular_width_deg,path_deg,ideal_deg,"
                "time_on_target_pct,mean_reaction_s,tracking_jitter_deg,reaction_count,"
                "perpendicular_jitter_deg,jitter_sample_s\n";
    }

    const bool full = mode == Mode::Full;
    goal_ = full ? 25 : 10;
    switch (test_) {
        case Test::Micro:
            microDistance_ = Distance;
            SpawnMicro();
            break;
        case Test::Flick: SpawnAnchor(); break;
        case Test::Switch:
            goal_ = full ? 30 : 12;
            SpawnCluster();
            break;
        case Test::Tracking:
            phaseSeconds_ = full ? 10.0 : 5.0;
            BeginTracking();
            break;
    }
}

void Benchmark::Stop() {
    if (IsRunning()) {
        Finish(false);
    }
    active_ = false;
    finished_ = false;
    targetCount_ = 0;
}

void Benchmark::SetPaused(bool paused, double now) {
    if (paused != paused_) {
        if (paused) {
            Advance(now);
        }
        paused_ = paused;
        lastTime_ = now;
    }
}

void Benchmark::Advance(double now) {
    if (!IsRunning() || paused_ || now <= lastTime_) {
        return;
    }
    const double end = elapsed_ + now - lastTime_;
    lastTime_ = now;
    if (test_ == Test::Tracking) {
        AdvanceTracking(std::min(end, 2.0 * phaseSeconds_));
        if (elapsed_ >= 2.0 * phaseSeconds_) {
            Finish(true);
        }
    } else {
        elapsed_ = end;
    }
}

std::optional<Vector3> Benchmark::Observe(MouseSample sample) {
    Advance(sample.time);
    if (!IsRunning() || paused_) {
        return std::nullopt;
    }
    const auto previous = look_;
    look_ = sample.look;
    if (test_ == Test::Tracking) {
        return ObserveTracking(previous, sample.clicked);
    }
    for (int i = 0; i < targetCount_; ++i) {
        targets_[i].motion.Observe(previous, look_);
    }
    return sample.clicked ? ClickTarget() : std::nullopt;
}

double Benchmark::Uniform(double low, double high) {
    return std::uniform_real_distribution<double>(low, high)(random_);
}

int Benchmark::RandomSign() {
    return Uniform(0.0, 1.0) < 0.5 ? -1 : 1;
}

void Benchmark::AddTarget(Vector3 center, bool anchor) {
    const auto target = AimAngles(center);
    const auto delta = Difference(target, look_);
    targets_[targetCount_++] = {
        center, {look_, target, elapsed_, std::hypot(delta.yaw, delta.pitch)}, nextId_++, anchor
    };
}

void Benchmark::SpawnMicro() {
    // Keep the head on the 165 cm plane. The yaw-biased trials retain range;
    // the remaining trials vary depth, which introduces a small pitch component.
    if (Uniform(0.0, 1.0) >= 0.7) {
        microDistance_ = Uniform(1500.0, 2500.0);
    }
    const double pitch = std::asin((HeadHeight - EyeHeight) / microDistance_) / Radians;
    const double pitchDelta = pitch - look_.pitch;
    const double distance = Uniform(1.5, 4.0);
    const double yawDelta = std::sqrt(std::max(0.0, distance * distance - pitchDelta * pitchDelta));
    int sign = RandomSign();
    if (std::abs(look_.yaw + sign * yawDelta) > 35.0) {
        sign = -sign;
    }
    auto center = PointAt({look_.yaw + sign * yawDelta, pitch}, microDistance_);
    center.y = HeadHeight;
    AddTarget(center);
}

void Benchmark::SpawnAnchor() {
    AddTarget(PointAt({}, Distance), true);
}

void Benchmark::SpawnFlick() {
    const double offset = Uniform(12.0, 25.0);
    const double pitch = Uniform(-3.0, 6.0);
    const double yaw = RandomSign() * std::sqrt(offset * offset - pitch * pitch);
    AddTarget(PointAt({yaw, pitch}, Distance));
}

void Benchmark::SpawnCluster() {
    const double pitch = std::clamp(look_.pitch, 1.0, 3.0);
    const double pitchDelta = pitch - look_.pitch;
    const double distance = Uniform(15.0, 25.0);
    const double yaw = look_.yaw +
        RandomSign() * std::sqrt(distance * distance - pitchDelta * pitchDelta);
    const double radius = Uniform(4.0, 8.0) / std::sqrt(3.0);
    const double rotation = Uniform(0.0, 2.0 * std::numbers::pi);
    for (int i = 0; i < 3; ++i) {
        const double angle = rotation + i * 2.0 * std::numbers::pi / 3.0;
        AddTarget(PointAt({yaw + radius * std::cos(angle), pitch + radius * std::sin(angle)},
                          Distance));
    }
}

std::optional<Vector3> Benchmark::ClickTarget() {
    const Ray ray{EyePosition, AimDirection(look_)};
    const float radius = ClickTargetRadius(test_);
    RayCollision contact{};
    int index = 0;
    for (; index < targetCount_; ++index) {
        const auto& target = targets_[index];
        contact = GetRayCollisionSphere(ray, target.center, radius);
        if (contact.hit) {
            break;
        }
    }
    if (index == targetCount_) {
        // Assign a miss to the nearest visible target without ending the attempt.
        const auto nearest = std::min_element(targets_.begin(), targets_.begin() + targetCount_,
            [this](const Target& a, const Target& b) {
                const auto da = Difference(a.motion.target, look_);
                const auto db = Difference(b.motion.target, look_);
                return std::hypot(da.yaw, da.pitch) < std::hypot(db.yaw, db.pitch);
            });
        const double width = AngularWidth(nearest->center, radius);
        Record("miss", nearest->id, false, nearest->motion, &look_, width);
        if (!nearest->anchor) {
            const auto& motion = nearest->motion;
            shots_.push_back({elapsed_ - motion.started, motion.Project(look_),
                              motion.maxProjection, motion.ideal, width, false, nearest->id});
        }
        return std::nullopt;
    }
    const auto target = targets_[index];
    const auto& motion = target.motion;
    const double width = AngularWidth(target.center, radius);
    Record(target.anchor ? "anchor" : "target", target.id, true, motion, &look_, width);
    targets_[index] = targets_[--targetCount_];
    if (target.anchor) {
        SpawnFlick();
        return contact.point;
    }

    const double projection = motion.Project(look_);
    shots_.push_back({elapsed_ - motion.started, projection, motion.maxProjection,
                      motion.ideal, width, true, target.id});
    hits_.push_back({elapsed_ - motion.started, motion.ideal, motion.actual, motion.samples > 0 ?
                         std::sqrt(motion.perpendicularSquared / motion.samples) : 0.0,
                     motion.maxProjection > motion.ideal + width / 2.0 &&
                         projection <= motion.ideal + width / 2.0});
    if (++completed_ == goal_) {
        Finish(true);
    } else if (test_ == Test::Micro) {
        SpawnMicro();
    } else if (test_ == Test::Flick) {
        SpawnAnchor();
    } else if (targetCount_ == 0) {
        SpawnCluster();
    } else {
        for (int i = 0; i < targetCount_; ++i) {
            auto& pending = targets_[i].motion;
            const auto delta = Difference(pending.target, look_);
            pending = {look_, pending.target, elapsed_, std::hypot(delta.yaw, delta.pitch)};
        }
    }
    return contact.point;
}

void Benchmark::BeginTracking() {
    trackingX_ = 0.0;
    direction_ = RandomSign();
    velocity_ = direction_ * StrafeSpeed;
    nextReverse_ = phaseSeconds_ + Uniform(0.8, 1.6);
    nextJump_ = phaseSeconds_ + Uniform(1.0, 2.0);
    jumpStart_ = -1.0;
    jumpHeight_ = 0.0;
    reactionStart_ = -1.0;
    reactionMotion_ = 0.0;
    reactionSum_ = 0.0;
    reactionCount_ = 0;
    braking_ = false;
    onTarget_ = 0.0;
    filteredError_ = Difference(look_, AimAngles(TrackingHead()));
    jitterIntegral_ = 0.0;
    perpendicularBaseline_ = filteredError_.pitch;
    perpendicularBand_ = 0.0;
    perpendicularSquares_ = 0.0;
    perpendicularSeconds_ = 0.0;
    trackingStart_ = look_;
    trackingStarted_ = 0.0;
    trackingPath_.clear();
    trackingPath_.reserve(10000);
}

void Benchmark::AdvanceTracking(double end) {
    constexpr double simulationStep = 1.0 / 240.0;
    while (elapsed_ < end) {
        if (!IsReactive()) {
            const double boundary = direction_ * SmoothLimit;
            const double toWall = std::max(0.0, (boundary - trackingX_) / velocity_);
            const double step = std::min({end - elapsed_, phaseSeconds_ - elapsed_,
                                          simulationStep, toWall});
            trackingX_ += velocity_ * step;
            elapsed_ += step;
            if (step == toWall) {
                trackingX_ = boundary;
                direction_ = -direction_;
                velocity_ = -velocity_;
            }
            UpdateTrackingMetrics(step);
            continue;
        }

        if (elapsed_ >= nextReverse_) {
            direction_ = -direction_;
            nextReverse_ += Uniform(0.8, 1.6);
            braking_ = true;
            reactionStart_ = -1.0;
        }
        if (elapsed_ >= nextJump_) {
            jumpStart_ = elapsed_;
            nextJump_ += Uniform(2.0, 3.5);
        }
        const double step = std::min({end - elapsed_, simulationStep,
                                      nextReverse_ - elapsed_, nextJump_ - elapsed_});
        const double desired = direction_ * StrafeSpeed;
        const double oldVelocity = velocity_;
        velocity_ += std::clamp(desired - velocity_, -StrafeAcceleration * step,
                               StrafeAcceleration * step);
        const double timeAccelerating = std::abs(velocity_ - oldVelocity) / StrafeAcceleration;
        trackingX_ += (oldVelocity + velocity_) * 0.5 * timeAccelerating +
                      velocity_ * (step - timeAccelerating);
        if (braking_ && direction_ * velocity_ >= 0.0) {
            braking_ = false;
            reactionStart_ = elapsed_ + std::abs(oldVelocity) / StrafeAcceleration;
            reactionMotion_ = 0.0;
        }
        elapsed_ += step;
        if (jumpStart_ >= 0.0) {
            const double flight = std::clamp((elapsed_ - jumpStart_) / 0.48, 0.0, 1.0);
            jumpHeight_ = 4.0 * 30.0 * flight * (1.0 - flight);
        }
        UpdateTrackingMetrics(step);
    }
}

Vector3 Benchmark::TrackingHead() const {
    return {static_cast<float>(trackingX_), HeadHeight + static_cast<float>(jumpHeight_),
            -static_cast<float>(Distance)};
}

std::optional<Vector3> Benchmark::TrackingContact() const {
    const Ray ray{EyePosition, AimDirection(look_)};
    const auto head = GetRayCollisionSphere(ray, TrackingHead(), TrackingHeadRadius);
    if (head.hit) {
        return head.point;
    }
    if (HitsBody(ray, static_cast<float>(trackingX_), static_cast<float>(jumpHeight_))) {
        const float travel = (BodyRadius - static_cast<float>(Distance)) / ray.direction.z;
        return Vector3{ray.direction.x * travel, EyeHeight + ray.direction.y * travel,
                       ray.direction.z * travel};
    }
    return std::nullopt;
}

void Benchmark::UpdateTrackingMetrics(double dt) {
    if (TrackingContact()) {
        onTarget_ += dt;
    }
    const auto center = AimAngles(TrackingHead());
    const auto error = Difference(look_, center);
    const auto residual = Difference(error, filteredError_);
    const double alpha = 1.0 - std::exp(-dt / 0.10);
    filteredError_.yaw += alpha * residual.yaw;
    filteredError_.pitch += alpha * residual.pitch;
    jitterIntegral_ += dt * (residual.yaw * residual.yaw + residual.pitch * residual.pitch);

    // Use the normal to the Smooth trajectory so intentional horizontal strafe
    // and the Reactive jumps do not dominate the calibration jitter estimate.
    if (elapsed_ <= phaseSeconds_) {
        const double height = (HeadHeight - EyeHeight) / Distance;
        const double yaw = center.yaw * Radians;
        const double tangent = -height * std::sin(yaw) /
            (1.0 + std::pow(height * std::cos(yaw), 2));
        const double perpendicular = (error.pitch - tangent * error.yaw) / std::hypot(1.0, tangent);
        const double slowAlpha = -std::expm1(-dt * 2.0 * std::numbers::pi * 7.0);
        const double fastAlpha = -std::expm1(-dt * 2.0 * std::numbers::pi * 14.0);
        perpendicularBaseline_ += slowAlpha * (perpendicular - perpendicularBaseline_);
        perpendicularBand_ += fastAlpha * (perpendicular - perpendicularBaseline_ - perpendicularBand_);
        if (std::abs(error.yaw) <= 2.0 && std::abs(perpendicular) <= 1.0) {
            perpendicularSquares_ += dt * perpendicularBand_ * perpendicularBand_;
            perpendicularSeconds_ += dt;
        }
    }
}

std::optional<Vector3> Benchmark::ObserveTracking(LookAngles previous, bool clicked) {
    trackingPath_.push_back(look_);
    if (reactionStart_ >= 0.0) {
        reactionMotion_ = std::max(0.0, reactionMotion_ -
            direction_ * Difference(look_, previous).yaw);
        if (reactionMotion_ >= 0.10) {
            reactionSum_ += elapsed_ - reactionStart_;
            ++reactionCount_;
            reactionStart_ = -1.0;
        }
    }
    if (clicked) {
        const auto head = TrackingHead();
        const auto goal = AimAngles(head);
        const auto delta = Difference(goal, trackingStart_);
        Motion motion{trackingStart_, goal, trackingStarted_, std::hypot(delta.yaw, delta.pitch)};
        auto last = trackingStart_;
        for (auto sample : trackingPath_) {
            motion.Observe(last, sample);
            last = sample;
        }
        const auto contact = TrackingContact();
        Record("tracking_click", 1, contact.has_value(), motion, &look_,
               AngularWidth(head, TrackingHeadRadius));
        trackingPath_.clear();
        trackingStart_ = look_;
        trackingStarted_ = elapsed_;
        return contact;
    }
    return std::nullopt;
}

void Benchmark::Record(const char* event, int id, bool hit, const Motion& motion,
                       const LookAngles* click, double width) {
    if (logFailed_) {
        return;
    }
    log_ << TestName(test_) << ',' << ModeName(mode_) << ',' << sensitivity_ << ',' << event << ','
         << id << ',';
    if (click) {
        log_ << static_cast<int>(hit);
    }
    log_ << ',' << motion.start.yaw << ',' << motion.start.pitch << ','
         << motion.target.yaw << ',' << motion.target.pitch << ',';
    if (click) {
        log_ << click->yaw << ',' << click->pitch << ',' << elapsed_ - motion.started << ','
             << motion.maxProjection << ',' << motion.Project(*click) << ',' << width << ','
             << motion.actual << ',' << motion.ideal << ",,,,,,\n";
    } else {
        log_ << ",," << elapsed_ << ",,,,,," << result_.timeOnTarget * 100.0 << ',';
        if (result_.reactions > 0) {
            log_ << result_.meanReaction;
        }
        log_ << ',' << result_.jitter << ',' << result_.reactions << ','
             << result_.perpendicularJitter << ',' << result_.jitterSeconds << '\n';
    }
    log_.flush();
    logFailed_ = !log_;
}

void Benchmark::Finish(bool completed) {
    if (test_ == Test::Tracking) {
        if (elapsed_ > 0.0) {
            result_.timeOnTarget = onTarget_ / elapsed_;
            result_.jitter = std::sqrt(jitterIntegral_ / elapsed_);
        }
        result_.reactions = reactionCount_;
        result_.jitterSeconds = perpendicularSeconds_;
        if (perpendicularSeconds_ > 0.0) {
            result_.perpendicularJitter = std::sqrt(perpendicularSquares_ / perpendicularSeconds_);
        }
        if (reactionCount_ > 0) {
            result_.meanReaction = reactionSum_ / reactionCount_;
        }
        Record(completed ? "tracking_end" : "tracking_stopped", 1, false,
               {{}, AimAngles(TrackingHead())}, nullptr, 0.0);
    } else if (!hits_.empty()) {
        const double count = static_cast<double>(hits_.size());
        double totalIdeal = 0.0;
        double totalActual = 0.0;
        double switchDelay = 0.0;
        int switches = 0;
        for (std::size_t i = 0; i < hits_.size(); ++i) {
            const auto& hit = hits_[i];
            result_.jitter += hit.jitter / count;
            result_.correctedOvershoots += static_cast<int>(hit.correctedOvershoot);
            totalIdeal += hit.ideal;
            totalActual += hit.actual;
            if (i % 3 != 0) {
                switchDelay += hit.mt;
                ++switches;
            }
        }
        result_.aim = AnalyzeAim(shots_);
        if (totalActual > 0.0) {
            result_.pathEfficiency = totalIdeal / totalActual;
        }
        if (switches > 0) {
            result_.meanSwitchDelay = switchDelay / switches;
        }
    }
    if (log_.is_open()) {
        log_.close();
        logFailed_ = logFailed_ || log_.fail();
    }
    finished_ = completed;
}

void Benchmark::Draw(const RangePalette& palette) const {
    if (!IsRunning()) {
        return;
    }
    if (test_ == Test::Tracking) {
        const auto head = TrackingHead();
        const float baseY = static_cast<float>(jumpHeight_);
        const bool onTarget = TrackingContact().has_value();
        DrawCapsule({head.x, baseY + BodyRadius, head.z},
                    {head.x, baseY + BodyHeight - BodyRadius, head.z},
                    BodyRadius, 16, 8, onTarget ? palette.trackingOnTarget : palette.trackingBody);
        DrawSphere(head, TrackingHeadRadius, onTarget ? palette.trackingOnTarget : palette.target);
        return;
    }
    for (int i = 0; i < targetCount_; ++i) {
        const auto& target = targets_[i];
        DrawSphere(target.center, ClickTargetRadius(test_),
                   target.anchor ? palette.anchor : palette.target);
    }
}
