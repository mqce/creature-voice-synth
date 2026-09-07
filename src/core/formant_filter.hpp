#pragma once

class CascadeFormants {
public:
    explicit CascadeFormants(float sample_rate);

    void set_formants(float f1, float f2, float f3, float f4, int ramp_samples);
    void set_nasal(float nasality, int ramp_samples);
    float process(float x);
    void reset();

private:
    struct Resonator {
        float a = 1.f;
        float b = 0.f;
        float c = 0.f;
        float a_inc = 0.f;
        float b_inc = 0.f;
        float c_inc = 0.f;
        float y1 = 0.f;
        float y2 = 0.f;
        float x1 = 0.f;
        float x2 = 0.f;
        int remaining = 0;

        void set_pole(float freq, float bw, float sample_rate, int ramp_samples);
        void set_zero(float freq, float bw, float sample_rate, int ramp_samples);
        void step_coeffs();
        float process_pole(float x);
        float process_zero(float x);
        void reset();
    };

    float sample_rate_ = 24000.f;
    float nasality_ = 0.f;
    float nasality_inc_ = 0.f;
    int nasality_remaining_ = 0;
    Resonator r_[4];
    Resonator nasal_pole_;
    Resonator nasal_zero_;
};
