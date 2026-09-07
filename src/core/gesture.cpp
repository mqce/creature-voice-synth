#include "gesture.hpp"

#include <algorithm>
#include <cmath>

namespace {
unsigned xorshift(unsigned& s) {
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return s;
}

float rand01(unsigned& s) {
    return static_cast<float>(xorshift(s) & 0xFFFFFFu) / 16777216.f;
}

float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

void scale_curve(Curve& c, float mul, float add, float lo, float hi) {
    for (int i = 0; i < c.n; ++i) {
        c.v[i] = std::clamp(c.v[i] * mul + add, lo, hi);
    }
}

void warp_interior_times(Curve& c, unsigned& rng, float amount) {
    if (c.n < 3) {
        return;
    }
    for (int i = 1; i < c.n - 1; ++i) {
        const float lo = c.t[i - 1] + 0.05f;
        const float hi = c.t[i + 1] - 0.05f;
        if (hi <= lo) {
            continue;
        }
        c.t[i] = std::clamp(c.t[i] + lerp(-amount, amount, rand01(rng)), lo, hi);
    }
}

void scale_contour_depth(Curve& c, float depth, float lo, float hi) {
    if (c.n <= 0) {
        return;
    }
    float sum = 0.f;
    for (int i = 0; i < c.n; ++i) {
        sum += c.v[i];
    }
    const float mean = sum / static_cast<float>(c.n);
    for (int i = 0; i < c.n; ++i) {
        c.v[i] = std::clamp(mean + (c.v[i] - mean) * depth, lo, hi);
    }
}

void jitter_interior_values(Curve& c, unsigned& rng, float amount, float lo, float hi, bool keep_ends) {
    const int begin = keep_ends ? 1 : 0;
    const int end = keep_ends ? c.n - 1 : c.n;
    for (int i = begin; i < end; ++i) {
        c.v[i] = std::clamp(c.v[i] + lerp(-amount, amount, rand01(rng)), lo, hi);
    }
}
}  // namespace

VocalGesture gesture_template(GestureKind kind) {
    VocalGesture g;
    switch (kind) {
    case GestureKind::Query:
        g.duration = 0.27f;
        g.pitch = make_curve({0.f, 546.f, 0.35f, 666.f, 1.f, 851.f});
        g.amplitude = make_curve({0.f, 0.f, 0.12f, 1.f, 0.72f, 0.85f, 1.f, 0.f});
        g.mouth = make_curve({0.f, 0.28f, 1.f, 0.52f});
        g.breath = make_curve({0.f, 0.13f, 1.f, 0.13f});
        g.jitter = 0.26f;
        g.open_quotient = 0.72f;
        g.nasality = 0.14f;
        g.roughness = 0.f;
        break;
    case GestureKind::Content:
        g.duration = 0.17f;
        g.pitch = make_curve({0.f, 599.f, 0.40f, 635.f, 1.f, 586.f});
        g.amplitude = make_curve({0.f, 0.f, 0.15f, 1.f, 0.70f, 0.80f, 1.f, 0.f});
        g.mouth = make_curve({0.f, 0.38f, 1.f, 0.44f});
        g.breath = make_curve({0.f, 0.11f, 1.f, 0.11f});
        g.jitter = 0.20f;
        g.open_quotient = 0.72f;
        g.nasality = 0.10f;
        g.roughness = 0.f;
        break;
    case GestureKind::Call:
        g.duration = 0.27f;
        g.pitch = make_curve({0.f, 540.f, 0.20f, 702.f, 0.80f, 688.f, 1.f, 648.f});
        g.amplitude = make_curve({0.f, 0.f, 0.10f, 1.f, 0.85f, 0.90f, 1.f, 0.f});
        g.mouth = make_curve({0.f, 0.36f, 0.50f, 0.52f, 1.f, 0.42f});
        g.breath = make_curve({0.f, 0.14f, 1.f, 0.14f});
        g.jitter = 0.28f;
        g.open_quotient = 0.71f;
        g.nasality = 0.10f;
        g.roughness = 0.f;
        break;
    case GestureKind::Coo:
        g.duration = 0.30f;
        g.pitch = make_curve({0.f, 493.f, 0.50f, 483.f, 1.f, 473.f});
        g.amplitude = make_curve({0.f, 0.f, 0.18f, 0.85f, 0.80f, 0.70f, 1.f, 0.f});
        g.mouth = make_curve({0.f, 0.22f, 1.f, 0.28f});
        g.breath = make_curve({0.f, 0.16f, 0.70f, 0.22f, 1.f, 0.36f});
        g.jitter = 0.16f;
        g.open_quotient = 0.78f;
        g.nasality = 0.29f;
        g.roughness = 0.f;
        break;
    case GestureKind::Sad:
        g.duration = 0.36f;
        g.pitch = make_curve({0.f, 435.f, 0.45f, 400.f, 1.f, 370.f});
        g.amplitude = make_curve({0.f, 0.f, 0.20f, 0.75f, 0.75f, 0.55f, 1.f, 0.f});
        g.mouth = make_curve({0.f, 0.24f, 1.f, 0.20f});
        g.breath = make_curve({0.f, 0.26f, 0.65f, 0.34f, 1.f, 0.48f});
        g.jitter = 0.22f;
        g.open_quotient = 0.78f;
        g.nasality = 0.20f;
        g.roughness = 0.f;
        break;
    case GestureKind::Excited:
        g.duration = 0.15f;
        g.pitch = make_curve({0.f, 716.f, 0.35f, 892.f, 1.f, 974.f});
        g.amplitude = make_curve({0.f, 0.f, 0.08f, 1.f, 0.55f, 0.90f, 1.f, 0.f});
        g.mouth = make_curve({0.f, 0.45f, 1.f, 0.58f});
        g.breath = make_curve({0.f, 0.16f, 1.f, 0.18f});
        g.jitter = 0.42f;
        g.open_quotient = 0.65f;
        g.nasality = 0.08f;
        g.roughness = 0.10f;
        break;
    case GestureKind::Chirp:
        g.duration = 0.11f;
        g.pitch = make_curve({0.f, 710.f, 1.f, 1056.f});
        g.amplitude = make_curve({0.f, 0.f, 0.10f, 1.f, 0.55f, 0.70f, 1.f, 0.f});
        g.mouth = make_curve({0.f, 0.36f, 1.f, 0.55f});
        g.breath = make_curve({0.f, 0.16f, 1.f, 0.16f});
        g.jitter = 0.34f;
        g.open_quotient = 0.70f;
        g.nasality = 0.10f;
        g.roughness = 0.f;
        break;
    case GestureKind::Whine:
        g.duration = 0.39f;
        g.pitch = make_curve({0.f, 756.f, 0.30f, 675.f, 1.f, 432.f});
        g.amplitude = make_curve({0.f, 0.f, 0.12f, 0.80f, 0.70f, 0.60f, 1.f, 0.f});
        g.mouth = make_curve({0.f, 0.32f, 1.f, 0.22f});
        g.breath = make_curve({0.f, 0.28f, 0.60f, 0.38f, 1.f, 0.52f});
        g.jitter = 0.38f;
        g.open_quotient = 0.79f;
        g.nasality = 0.16f;
        g.roughness = 0.13f;
        break;
    }
    return g;
}

VocalGesture instantiate_gesture(GestureKind kind, unsigned& rng, const GestureScale& scale) {
    VocalGesture g = gesture_template(kind);

    const float dur_r = lerp(0.70f, 1.32f, rand01(rng));
    const float f0_r = lerp(0.92f, 1.13f, rand01(rng));
    const float depth = lerp(0.58f, 1.58f, rand01(rng));
    const float mouth_r = lerp(-0.18f, 0.18f, rand01(rng));
    const float mouth_span = lerp(0.55f, 1.45f, rand01(rng));
    const float breath_r = lerp(-0.14f, 0.22f, rand01(rng));
    const float jitter_r = lerp(-0.12f, 0.28f, rand01(rng));
    const float amp_peak = lerp(0.72f, 1.0f, rand01(rng));

    g.duration = std::clamp(g.duration * dur_r * scale.duration_scale, 0.07f, 1.2f);

    warp_interior_times(g.pitch, rng, 0.16f);
    warp_interior_times(g.amplitude, rng, 0.14f);
    warp_interior_times(g.mouth, rng, 0.18f);

    scale_contour_depth(g.pitch, depth, 380.f, 1300.f);
    jitter_interior_values(g.pitch, rng, 40.f, 380.f, 1300.f, true);
    scale_curve(g.pitch, f0_r * scale.pitch_scale, 0.f, 380.f, 1300.f);

    scale_contour_depth(g.mouth, mouth_span, 0.f, 1.f);
    scale_curve(g.mouth, 1.f, mouth_r + scale.mouth_bias, 0.f, 1.f);

    // Keep attack/release at 0; vary how loud the body is.
    for (int i = 1; i < g.amplitude.n - 1; ++i) {
        g.amplitude.v[i] = std::clamp(g.amplitude.v[i] * amp_peak, 0.12f, 1.f);
    }
    if (g.amplitude.n > 0) {
        g.amplitude.v[0] = 0.f;
        g.amplitude.v[g.amplitude.n - 1] = 0.f;
    }

    scale_curve(g.breath, 1.f, scale.breath_add + breath_r, 0.f, 1.f);
    warp_interior_times(g.breath, rng, 0.12f);
    g.jitter = std::clamp(g.jitter + scale.jitter_add + jitter_r, 0.f, 1.f);
    g.open_quotient = std::clamp(g.open_quotient + lerp(-0.05f, 0.05f, rand01(rng)), 0.45f, 0.88f);
    g.nasality = std::clamp(g.nasality + lerp(-0.08f, 0.08f, rand01(rng)), 0.f, 1.f);
    if (g.roughness > 0.001f) {
        g.roughness = std::clamp(g.roughness + lerp(-0.04f, 0.05f, rand01(rng)), 0.04f, 0.22f);
    } else {
        g.roughness = 0.f;
    }
    return g;
}

const char* gesture_kind_name(GestureKind kind) {
    switch (kind) {
    case GestureKind::Query:
        return "Query";
    case GestureKind::Content:
        return "Content";
    case GestureKind::Call:
        return "Call";
    case GestureKind::Coo:
        return "Coo";
    case GestureKind::Sad:
        return "Sad";
    case GestureKind::Excited:
        return "Excited";
    case GestureKind::Chirp:
        return "Chirp";
    case GestureKind::Whine:
        return "Whine";
    }
    return "?";
}
