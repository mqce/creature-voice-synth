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
    mouth_sm_.set_time_sec(0.010f, sample_rate_);
    jitter_sm_.set_time_sec(0.020f, sample_rate_);
    amp_sm_.set_time_sec(0.005f, sample_rate_);
    oq_sm_.set_time_sec(0.012f, sample_rate_);
    nasal_sm_.set_time_sec(0.020f, sample_rate_);
    rough_sm_.set_time_sec(0.020f, sample_rate_);

    const VoiceParams d{};
    pitch_sm_.snap(d.pitch_hz);
    breath_sm_.snap(d.breath);
    mouth_sm_.snap(d.mouth);
    jitter_sm_.snap(d.jitter);
    amp_sm_.snap(0.f);
    oq_sm_.snap(d.open_quotient);
    nasal_sm_.snap(d.nasality);
    rough_sm_.snap(d.roughness);

    float f1, f2, f3, f4;
    map_mouth(d.mouth, f1, f2, f3, f4);
    formants_.set_formants(f1, f2, f3, f4, 1);
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

void VoiceEngine::map_mouth(float mouth, float& f1, float& f2, float& f3, float& f4) const {
    mouth = std::clamp(mouth, 0.f, 1.f);
    f1 = 650.f + mouth * 470.f;     // ~650–1120
    f2 = 2480.f - mouth * 660.f;    // ~2480–1820
    f3 = 3575.f + mouth * 80.f;
    f4 = 4870.f;
}

void VoiceEngine::maybe_update_formants(float mouth, float nasality) {
    float f1, f2, f3, f4;
    map_mouth(mouth, f1, f2, f3, f4);
    formants_.set_formants(f1, f2, f3, f4, frame_len_);
    formants_.set_nasal(nasality, frame_len_);
}

float VoiceEngine::tick(const VoiceParams& target) {
    if (samples_in_frame_ >= frame_len_) {
        samples_in_frame_ = 0;
        maybe_update_formants(mouth_sm_.value(), nasal_sm_.value());
    }
    ++samples_in_frame_;

    const bool playing = playing_.load(std::memory_order_relaxed);
    const float amp_target = playing ? target.amplitude : 0.f;

    const float pitch = pitch_sm_.process(target.pitch_hz);
    const float breath = breath_sm_.process(target.breath);
    mouth_sm_.process(target.mouth);
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
