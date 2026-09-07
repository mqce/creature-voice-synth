#pragma once

class GlottalSource {
public:
    explicit GlottalSource(float sample_rate);

    // Rosenberg pulse differentiated at 2x sample rate, then downsampled.
    float process(float pitch_hz, float jitter, float open_quotient, float roughness);

    void reset();

private:
    float pulse(float phase) const;
    float next_period_samples(float pitch_hz, float jitter);
    void wrap_period(float pitch_hz, float jitter, float open_quotient, float roughness);

    float sample_rate_ = 24000.f;
    float phase_ = 0.f;
    float phase_half_ = 0.f;
    float period_samples_ = 57.f;
    float prev_flow_ = 0.f;
    float open_quotient_ = 0.68f;
    float shimmer_gain_ = 1.f;
    unsigned rng_ = 0xA341316Cu;
};
