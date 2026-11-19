#pragma once

#include "emotion.hpp"
#include "gesture.hpp"
#include "voice_params.hpp"

#include <atomic>

class GesturePlayer {
public:
    explicit GesturePlayer(float sample_rate);

    void trigger(const VocalGesture& gesture);
    void trigger_kind(GestureKind kind);
    void trigger_emotion(const Emotion& emotion);
    void trigger_diphthong(VowelKind from, VowelKind to);

    VoiceParams next_params();

private:
    float sample_rate_ = 24000.f;
    VocalGesture slots_[2]{};
    std::atomic<int> pub_{0};
    std::atomic<unsigned> serial_{0};
    unsigned last_serial_ = 0;
    VocalGesture current_{};
    bool active_ = false;
    int pos_ = 0;
    int length_ = 0;
    VoiceParams last_{};
    unsigned rng_ = 0xC2B2AE35u;
};
