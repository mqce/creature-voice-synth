#pragma once

class BreathNoise {
public:
    explicit BreathNoise(float sample_rate = 24000.f);

    float process();

private:
    unsigned rng_ = 0xC0FFEE01u;
    float x1_ = 0.f;
    float y1_ = 0.f;
    float r_ = 0.73f;
};
