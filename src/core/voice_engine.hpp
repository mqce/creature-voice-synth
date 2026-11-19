#pragma once

#include "formant_filter.hpp"
#include "glottal_source.hpp"
#include "noise.hpp"
#include "param_smoother.hpp"
#include "voice_params.hpp"

#include <atomic>

class VoiceEngine {
public:
    explicit VoiceEngine(float sample_rate = 24000.f);

    void set_params(const VoiceParams& params);
    void set_playing(bool playing);

    void render(float* out, int n);
    float tick(const VoiceParams& target);

    float sample_rate() const { return sample_rate_; }

private:
    void maybe_update_formants(float f1, float f2, float f3, float nasality);

    float sample_rate_ = 24000.f;
    int frame_len_ = 192;
    int samples_in_frame_ = 0;

    VoiceParams mailbox_slots_[2]{};
    std::atomic<int> mailbox_pub_{0};
    std::atomic<bool> playing_{true};

    GlottalSource glottal_;
    BreathNoise noise_;
    CascadeFormants formants_;
    ParamSmoother pitch_sm_;
    ParamSmoother breath_sm_;
    ParamSmoother f1_sm_;
    ParamSmoother f2_sm_;
    ParamSmoother f3_sm_;
    ParamSmoother jitter_sm_;
    ParamSmoother amp_sm_;
    ParamSmoother oq_sm_;
    ParamSmoother nasal_sm_;
    ParamSmoother rough_sm_;

    float tilt_z_ = 0.f;
    float dc_x1_ = 0.f;
    float dc_y1_ = 0.f;
};
