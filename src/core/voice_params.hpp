#pragma once

// Synthesizer-layer controls, driven by the drone GUI or by Vocal Gesture.
struct VoiceParams {
    float pitch_hz = 690.f;       // 180–1100
    float breath = 0.16f;         // 0–1, mixed at the glottis
    float f1 = 1060.f;            // Hz, current vowel F1
    float f2 = 1580.f;            // Hz, current vowel F2
    float f3 = 3540.f;            // Hz, current vowel F3
    float jitter = 0.32f;         // 0–1, cycle-to-cycle F0 variation
    float amplitude = 1.f;        // 0–1
    float open_quotient = 0.74f;  // 0.45–0.88
    float nasality = 0.18f;       // 0–1
    float roughness = 0.f;        // 0–1, subharmonic mix
};
