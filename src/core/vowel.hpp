#pragma once

#include <algorithm>

enum class VowelKind { A = 0, I, U, E, O, Count };

struct VowelFormants {
    float f1 = 1060.f;
    float f2 = 1580.f;
    float f3 = 3540.f;
};

inline VowelFormants vowel_formants(VowelKind kind) {
    switch (kind) {
    case VowelKind::A:
        return {1060.f, 1580.f, 3540.f};
    case VowelKind::I:
        return {390.f, 3320.f, 4360.f};
    case VowelKind::U:
        return {435.f, 1260.f, 3250.f};
    case VowelKind::E:
        return {770.f, 2670.f, 3600.f};
    case VowelKind::O:
        return {830.f, 1220.f, 3500.f};
    default:
        return {1060.f, 1580.f, 3540.f};
    }
}

inline VowelFormants mix_vowels(const VowelFormants& a, const VowelFormants& b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    return {
        a.f1 + (b.f1 - a.f1) * t,
        a.f2 + (b.f2 - a.f2) * t,
        a.f3 + (b.f3 - a.f3) * t,
    };
}

inline const char* vowel_label(VowelKind kind) {
    switch (kind) {
    case VowelKind::A:
        return "あ";
    case VowelKind::I:
        return "い";
    case VowelKind::U:
        return "う";
    case VowelKind::E:
        return "え";
    case VowelKind::O:
        return "お";
    default:
        return "?";
    }
}

inline VowelKind vowel_from_index(int index) {
    index = std::clamp(index, 0, static_cast<int>(VowelKind::Count) - 1);
    return static_cast<VowelKind>(index);
}
