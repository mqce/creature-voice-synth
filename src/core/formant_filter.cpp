#include "formant_filter.hpp"

#include <algorithm>
#include <cmath>

namespace {
constexpr float kPi = 3.14159265358979323846f;
}

void CascadeFormants::Resonator::step_coeffs() {
    if (remaining > 0) {
        a += a_inc;
        b += b_inc;
        c += c_inc;
        --remaining;
    }
}

void CascadeFormants::Resonator::set_pole(float freq, float bw, float sample_rate, int ramp_samples) {
    const float nyquist = sample_rate * 0.45f;
    freq = std::clamp(freq, 50.f, nyquist);
    bw = std::clamp(bw, 40.f, freq * 0.5f);

    const float r = std::exp(-kPi * bw / sample_rate);
    const float new_c = -r * r;
    const float new_b = 2.f * r * std::cos(2.f * kPi * freq / sample_rate);
    const float new_a = 1.f - new_b - new_c;

    ramp_samples = std::max(ramp_samples, 1);
    a_inc = (new_a - a) / static_cast<float>(ramp_samples);
    b_inc = (new_b - b) / static_cast<float>(ramp_samples);
    c_inc = (new_c - c) / static_cast<float>(ramp_samples);
    remaining = ramp_samples;
}

void CascadeFormants::Resonator::set_zero(float freq, float bw, float sample_rate, int ramp_samples) {
    const float nyquist = sample_rate * 0.45f;
    freq = std::clamp(freq, 50.f, nyquist);
    bw = std::clamp(bw, 40.f, freq * 0.5f);

    const float r = std::exp(-kPi * bw / sample_rate);
    float new_c = -r * r;
    float new_b = 2.f * r * std::cos(2.f * kPi * freq / sample_rate);
    float new_a = 1.f - new_b - new_c;
    if (std::abs(new_a) < 1.0e-6f) {
        new_a = 1.0e-6f;
    }
    const float inv = 1.f / new_a;
    new_a = inv;
    new_b = -new_b * inv;
    new_c = -new_c * inv;

    ramp_samples = std::max(ramp_samples, 1);
    a_inc = (new_a - a) / static_cast<float>(ramp_samples);
    b_inc = (new_b - b) / static_cast<float>(ramp_samples);
    c_inc = (new_c - c) / static_cast<float>(ramp_samples);
    remaining = ramp_samples;
}

float CascadeFormants::Resonator::process_pole(float x) {
    step_coeffs();
    float y = a * x + b * y1 + c * y2;
    y2 = y1;
    y1 = y;
    if (!std::isfinite(y)) {
        y = 0.f;
        y1 = 0.f;
        y2 = 0.f;
    } else if (std::abs(y1) < 1.0e-20f) {
        y1 = 0.f;
        y2 = 0.f;
    }
    return y;
}

float CascadeFormants::Resonator::process_zero(float x) {
    step_coeffs();
    float y = a * x + b * x1 + c * x2;
    x2 = x1;
    x1 = x;
    if (!std::isfinite(y)) {
        y = 0.f;
        x1 = 0.f;
        x2 = 0.f;
    }
    return y;
}

void CascadeFormants::Resonator::reset() {
    a = 1.f;
    b = 0.f;
    c = 0.f;
    a_inc = b_inc = c_inc = 0.f;
    y1 = y2 = 0.f;
    x1 = x2 = 0.f;
    remaining = 0;
}

CascadeFormants::CascadeFormants(float sample_rate) : sample_rate_(sample_rate) {
    nasal_pole_.set_pole(270.f, 90.f, sample_rate_, 1);
    nasal_zero_.set_zero(450.f, 90.f, sample_rate_, 1);
}

void CascadeFormants::set_formants(float f1, float f2, float f3, float f4, int ramp_samples) {
    r_[0].set_pole(f1, 95.f, sample_rate_, ramp_samples);
    r_[1].set_pole(f2, 130.f, sample_rate_, ramp_samples);
    r_[2].set_pole(f3, 180.f, sample_rate_, ramp_samples);
    r_[3].set_pole(f4, 250.f, sample_rate_, ramp_samples);
}

void CascadeFormants::set_nasal(float nasality, int ramp_samples) {
    nasality = std::clamp(nasality, 0.f, 1.f);
    ramp_samples = std::max(ramp_samples, 1);
    nasality_inc_ = (nasality - nasality_) / static_cast<float>(ramp_samples);
    nasality_remaining_ = ramp_samples;
    nasal_pole_.set_pole(270.f, 90.f, sample_rate_, ramp_samples);
    nasal_zero_.set_zero(450.f, 90.f, sample_rate_, ramp_samples);
}

float CascadeFormants::process(float x) {
    float y = x;
    for (auto& r : r_) {
        y = r.process_pole(y);
    }
    if (nasality_remaining_ > 0) {
        nasality_ += nasality_inc_;
        --nasality_remaining_;
    }
    if (nasality_ > 0.001f) {
        const float n = nasal_zero_.process_zero(nasal_pole_.process_pole(y));
        y += nasality_ * (n - y);
    }
    return y;
}

void CascadeFormants::reset() {
    for (auto& r : r_) {
        r.reset();
    }
    nasal_pole_.reset();
    nasal_zero_.reset();
    nasality_ = 0.f;
    nasality_inc_ = 0.f;
    nasality_remaining_ = 0;
}
