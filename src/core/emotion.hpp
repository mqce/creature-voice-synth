#pragma once

#include "gesture.hpp"

struct Emotion {
    float valence = 0.25f;     // -1..1
    float arousal = 0.40f;     // 0..1
    float affection = 0.35f;   // 0..1
    float curiosity = 0.70f;   // 0..1
    float fear = 0.05f;        // 0..1
};

struct MappedGesture {
    GestureKind kind = GestureKind::Content;
    GestureScale scale{};
};

MappedGesture map_emotion(const Emotion& e);
