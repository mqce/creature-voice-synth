#pragma once

#include <algorithm>
#include <initializer_list>

struct Curve {
    int n = 0;
    float t[8]{};
    float v[8]{};

    float eval(float u) const {
        if (n <= 0) {
            return 0.f;
        }
        if (n == 1) {
            return v[0];
        }
        u = std::clamp(u, 0.f, 1.f);
        if (u <= t[0]) {
            return v[0];
        }
        for (int i = 1; i < n; ++i) {
            if (u <= t[i]) {
                const float den = t[i] - t[i - 1];
                const float a = (den > 1.0e-8f) ? (u - t[i - 1]) / den : 0.f;
                return v[i - 1] + a * (v[i] - v[i - 1]);
            }
        }
        return v[n - 1];
    }
};

inline Curve make_curve(std::initializer_list<float> tv) {
    Curve c;
    auto it = tv.begin();
    while (it != tv.end() && c.n < 8) {
        const float ti = *it++;
        if (it == tv.end()) {
            break;
        }
        c.t[c.n] = ti;
        c.v[c.n] = *it++;
        ++c.n;
    }
    return c;
}
