#pragma once

#include <array>

inline constexpr std::array MsaaOptions{0, 2, 4, 8};

class MultisampleTarget {
public:
    MultisampleTarget() = default;
    ~MultisampleTarget();
    MultisampleTarget(const MultisampleTarget&) = delete;
    MultisampleTarget& operator=(const MultisampleTarget&) = delete;

    bool Begin(int width, int height, int requestedSamples);
    void Resolve() const;
    int Samples() const { return samples_; }

private:
    bool Allocate(int requestedSamples);
    void Release();

    unsigned int framebuffer_ = 0;
    unsigned int color_ = 0;
    unsigned int depth_ = 0;
    int width_ = 0;
    int height_ = 0;
    int requested_ = -1;
    int samples_ = 0;
};
