#pragma once

#include "curve.hpp"
#include "vowel.hpp"

enum class GestureKind {
    Query,
    Content,
    Call,
    Coo,
    Sad,
    Excited,
    Chirp,
    Whine,
};

struct VocalGesture {
    float duration = 0.2f;
    Curve pitch;
    Curve amplitude;
    Curve breath;
    Curve vowel_mix;
    VowelKind vowel_from = VowelKind::A;
    VowelKind vowel_to = VowelKind::A;
    float f1_bias = 0.f;
    float f2_bias = 0.f;
    float jitter = 0.35f;
    float open_quotient = 0.68f;
    float nasality = 0.f;
    float roughness = 0.f;
};

struct GestureScale {
    float pitch_scale = 1.f;
    float duration_scale = 1.f;
    float breath_add = 0.f;
    float jitter_add = 0.f;
    float mouth_bias = 0.f;
};

VocalGesture gesture_template(GestureKind kind);
VocalGesture instantiate_gesture(GestureKind kind, unsigned& rng, const GestureScale& scale);
VocalGesture make_diphthong_gesture(VowelKind from, VowelKind to, unsigned& rng);
const char* gesture_kind_name(GestureKind kind);
const char* gesture_kind_label(GestureKind kind);
