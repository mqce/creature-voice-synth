#include "gesture_player.hpp"

#include <algorithm>

GesturePlayer::GesturePlayer(float sample_rate) : sample_rate_(sample_rate) {
    last_.amplitude = 0.f;
}

void GesturePlayer::trigger(const VocalGesture& gesture) {
    const int next = 1 - pub_.load(std::memory_order_relaxed);
    slots_[next] = gesture;
    pub_.store(next, std::memory_order_release);
    serial_.fetch_add(1, std::memory_order_release);
}

void GesturePlayer::trigger_kind(GestureKind kind) {
    trigger(instantiate_gesture(kind, rng_, GestureScale{}));
}

void GesturePlayer::trigger_emotion(const Emotion& emotion) {
    const MappedGesture mapped = map_emotion(emotion);
    trigger(instantiate_gesture(mapped.kind, rng_, mapped.scale));
}

VoiceParams GesturePlayer::next_params() {
    const unsigned serial = serial_.load(std::memory_order_acquire);
    if (serial != last_serial_) {
        last_serial_ = serial;
        current_ = slots_[pub_.load(std::memory_order_acquire)];
        length_ = std::max(1, static_cast<int>(current_.duration * sample_rate_));
        pos_ = 0;
        active_ = true;
    }

    if (!active_) {
        last_.amplitude = 0.f;
        return last_;
    }

    const float u = (length_ <= 1) ? 1.f : static_cast<float>(pos_) / static_cast<float>(length_ - 1);
    VoiceParams p;
    p.pitch_hz = current_.pitch.eval(u);
    p.mouth = current_.mouth.eval(u);
    p.amplitude = current_.amplitude.eval(u);
    p.breath = current_.breath.eval(u);
    p.jitter = current_.jitter;
    p.open_quotient = current_.open_quotient;
    p.nasality = current_.nasality;
    p.roughness = current_.roughness;
    last_ = p;

    ++pos_;
    if (pos_ >= length_) {
        active_ = false;
        last_.amplitude = 0.f;
    }
    return p;
}
