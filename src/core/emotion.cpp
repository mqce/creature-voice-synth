#include "emotion.hpp"

#include <algorithm>
#include <cmath>

MappedGesture map_emotion(const Emotion& e) {
    const float valence = std::clamp(e.valence, -1.f, 1.f);
    const float arousal = std::clamp(e.arousal, 0.f, 1.f);
    const float affection = std::clamp(e.affection, 0.f, 1.f);
    const float curiosity = std::clamp(e.curiosity, 0.f, 1.f);
    const float fear = std::clamp(e.fear, 0.f, 1.f);

    MappedGesture m;

    if (fear > 0.6f && arousal > 0.5f) {
        m.kind = (arousal > 0.75f) ? GestureKind::Chirp : GestureKind::Whine;
    } else if (curiosity > 0.55f) {
        m.kind = GestureKind::Query;
    } else if (valence < -0.4f) {
        m.kind = GestureKind::Sad;
    } else if (arousal > 0.7f && valence > 0.2f) {
        m.kind = GestureKind::Excited;
    } else if (affection > 0.6f && arousal < 0.4f) {
        m.kind = GestureKind::Coo;
    } else if (valence > 0.3f && arousal < 0.45f) {
        m.kind = GestureKind::Content;
    } else if (arousal > 0.5f) {
        m.kind = GestureKind::Call;
    } else {
        m.kind = GestureKind::Content;
    }

    m.scale.pitch_scale = 1.f + 0.25f * arousal + 0.10f * valence - 0.15f * fear;
    m.scale.duration_scale = 1.15f - 0.40f * arousal + 0.20f * std::max(0.f, -valence);
    m.scale.breath_add = 0.12f * fear + 0.08f * std::max(0.f, -valence);
    m.scale.jitter_add = 0.20f * fear + 0.12f * arousal;
    m.scale.mouth_bias = 0.08f * valence + 0.06f * curiosity - 0.08f * fear;
    return m;
}
