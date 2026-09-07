#include "glottal_source.hpp"

#include <algorithm>

namespace {
unsigned xorshift(unsigned& s) {
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return s;
}

float rand_uniform(unsigned& s) {
    return static_cast<float>(xorshift(s) & 0xFFFFFFu) / 16777216.f;
}
}  // namespace

GlottalSource::GlottalSource(float sample_rate) : sample_rate_(sample_rate) {
    period_samples_ = sample_rate_ / 690.f;
}

void GlottalSource::reset() {
    phase_ = 0.f;
    phase_half_ = 0.f;
    prev_flow_ = 0.f;
    shimmer_gain_ = 1.f;
}

float GlottalSource::pulse(float phase) const {
    const float tp = open_quotient_ * 0.6f;
    const float tn = open_quotient_ * 0.4f;
    if (phase < tp) {
        const float x = phase / tp;
        return (3.f - 2.f * x) * x * x;
    }
    if (phase < tp + tn) {
        const float x = (phase - tp) / tn;
        return 1.f - x * x;
    }
    return 0.f;
}

float GlottalSource::next_period_samples(float pitch_hz, float jitter) {
    pitch_hz = std::clamp(pitch_hz, 200.f, 1300.f);
    const float depth = 0.003f + std::clamp(jitter, 0.f, 1.f) * 0.017f;
    const float mul = 1.f + (rand_uniform(rng_) * 2.f - 1.f) * depth;
    return sample_rate_ / (pitch_hz * mul);
}

void GlottalSource::wrap_period(float pitch_hz, float jitter, float open_quotient, float roughness) {
    open_quotient_ = std::clamp(open_quotient, 0.45f, 0.88f);
    period_samples_ = next_period_samples(pitch_hz, jitter);
    const float shimmer = std::clamp(0.15f * jitter + 0.55f * roughness, 0.f, 1.f);
    const float depth = 0.02f + 0.12f * shimmer;
    shimmer_gain_ = 1.f + (rand_uniform(rng_) * 2.f - 1.f) * depth;
}

float GlottalSource::process(float pitch_hz, float jitter, float open_quotient, float roughness) {
    const float mix = std::clamp(roughness, 0.f, 1.f) * 0.35f;
    const float half = 0.5f / period_samples_;
    const float half2 = 0.25f / period_samples_;

    auto sample_flow = [&](float p, float p2) {
        return pulse(p) + mix * pulse(p2);
    };

    const float f0 = sample_flow(phase_, phase_half_);
    phase_ += half;
    phase_half_ += half2;
    if (phase_ >= 1.f) {
        phase_ -= 1.f;
        wrap_period(pitch_hz, jitter, open_quotient, roughness);
    }
    if (phase_half_ >= 1.f) {
        phase_half_ -= 1.f;
    }

    const float f1 = sample_flow(phase_, phase_half_);
    phase_ += half;
    phase_half_ += half2;
    if (phase_ >= 1.f) {
        phase_ -= 1.f;
        wrap_period(pitch_hz, jitter, open_quotient, roughness);
    }
    if (phase_half_ >= 1.f) {
        phase_half_ -= 1.f;
    }

    const float d0 = f0 - prev_flow_;
    const float d1 = f1 - f0;
    prev_flow_ = f1;

    return 0.5f * (d0 + d1) * period_samples_ * 0.35f * shimmer_gain_;
}
