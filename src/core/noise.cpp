#include "noise.hpp"

#include <cmath>

BreathNoise::BreathNoise(float sample_rate) {
    const float sr = (sample_rate > 1.f) ? sample_rate : 24000.f;
    r_ = std::exp(-2.f * 3.14159265f * 1200.f / sr);
}

float BreathNoise::process() {
    rng_ ^= rng_ << 13;
    rng_ ^= rng_ >> 17;
    rng_ ^= rng_ << 5;
    const float white = static_cast<float>(static_cast<int>(rng_) >> 8) * (1.f / 8388608.f);
    const float y = white - x1_ + r_ * y1_;
    x1_ = white;
    y1_ = y;
    return y;
}
