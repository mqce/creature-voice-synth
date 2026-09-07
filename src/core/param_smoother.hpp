#pragma once

#include <cmath>

class ParamSmoother {
public:
    void set_time_sec(float seconds, float sample_rate) {
        if (seconds <= 0.f) {
            coeff_ = 1.f;
            return;
        }
        coeff_ = 1.f - std::exp(-1.f / (seconds * sample_rate));
    }

    void snap(float value) { current_ = value; }

    float process(float target) {
        current_ += coeff_ * (target - current_);
        return current_;
    }

    float value() const { return current_; }

private:
    float current_ = 0.f;
    float coeff_ = 0.01f;
};
