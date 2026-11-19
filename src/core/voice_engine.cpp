#include "voice_engine.hpp"

#include <algorithm>
#include <cmath>

namespace {
constexpr float kTiltCoeff = 0.48f;
constexpr float kDcR = 0.995f;
constexpr float kCascadeGain = 0.22f;

float soft_clip(float x) {
    return std::tanh(x);
}
}  // namespace

VoiceEngine::VoiceEngine(float sample_rate)
    : sample_rate_(sample_rate),
      frame_len_(std::max(1, static_cast<int>(sample_rate * 0.008f))),
      glottal_(sample_rate),
      noise_(sample_rate),
      formants_(sample_rate) {
    mailbox_slots_[0] = VoiceParams{};
    mailbox_slots_[1] = VoiceParams{};

    pitch_sm_.set_time_sec(0.008f, sample_rate_);
    breath_sm_.set_time_sec(0.015f, sample_rate_);
    f1_sm_.set_time_sec(0.010f, sample_rate_);
    f2_sm_.set_time_sec(0.010f, sample_rate_);
    f3_sm_.set_time_sec(0.010f, sample_rate_);
    jitter_sm_.set_time_sec(0.020f, sample_rate_);
    amp_sm_.set_time_sec(0.005f, sample_rate_);
    oq_sm_.set_time_sec(0.012f, sample_rate_);
    nasal_sm_.set_time_sec(0.020f, sample_rate_);
    rough_sm_.set_time_sec(0.020f, sample_rate_);

    const VoiceParams d{};
    pitch_sm_.snap(d.pitch_hz);
    breath_sm_.snap(d.breath);
    f1_sm_.snap(d.f1);
    f2_sm_.snap(d.f2);
    f3_sm_.snap(d.f3);
    jitter_sm_.snap(d.jitter);
    amp_sm_.snap(0.f);
    oq_sm_.snap(d.open_quotient);
    nasal_sm_.snap(d.nasality);
    rough_sm_.snap(d.roughness);

    formants_.set_formants(d.f1, d.f2, d.f3, std::max(4870.f, d.f3 + 900.f), 1);
    formants_.set_nasal(d.nasality, 1);
}

void VoiceEngine::set_params(const VoiceParams& params) {
    const int next = 1 - mailbox_pub_.load(std::memory_order_relaxed);
    mailbox_slots_[next] = params;
    mailbox_pub_.store(next, std::memory_order_release);
}

void VoiceEngine::set_playing(bool playing) {
    playing_.store(playing, std::memory_order_release);
}

void VoiceEngine::maybe_update_formants(float f1, float f2, float f3, float nasality) {
    formants_.set_formants(f1, f2, f3, std::max(4870.f, f3 + 900.f), frame_len_);
    formants_.set_nasal(nasality, frame_len_);
}

float VoiceEngine::tick(const VoiceParams& target) {
    if (samples_in_frame_ >= frame_len_) {
        samples_in_frame_ = 0;
        maybe_update_formants(f1_sm_.value(), f2_sm_.value(), f3_sm_.value(), nasal_sm_.value());
    }
    ++samples_in_frame_;

    const bool playing = playing_.load(std::memory_order_relaxed);
    const float amp_target = playing ? target.amplitude : 0.f;

    const float pitch = pitch_sm_.process(target.pitch_hz);
    const float breath = breath_sm_.process(target.breath);
    f1_sm_.process(target.f1);
    f2_sm_.process(target.f2);
    f3_sm_.process(target.f3);
    const float jitter = jitter_sm_.process(target.jitter);
    const float amp = amp_sm_.process(amp_target);
    const float oq = oq_sm_.process(target.open_quotient);
    nasal_sm_.process(target.nasality);
    const float rough = rough_sm_.process(target.roughness);

    const float g = glottal_.process(pitch, jitter, oq, rough);
    const float aspiration = noise_.process() * breath * 0.28f;
    float v = formants_.process(g + aspiration) * kCascadeGain;

    tilt_z_ += kTiltCoeff * (v - tilt_z_);
    v = tilt_z_;

    const float dc = v - dc_x1_ + kDcR * dc_y1_;
    dc_x1_ = v;
    dc_y1_ = dc;
    v = dc;

    return soft_clip(v * 1.15f) * 0.55f * amp;
}

void VoiceEngine::render(float* out, int n) {
    const VoiceParams target = mailbox_slots_[mailbox_pub_.load(std::memory_order_acquire)];
    for (int i = 0; i < n; ++i) {
        out[i] = tick(target);
    }
}
