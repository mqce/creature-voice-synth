#include "emotion.hpp"
#include "gesture.hpp"
#include "gesture_player.hpp"
#include "voice_engine.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "miniaudio.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

namespace {
constexpr int kSampleRate = 24000;

enum class AppMode { Gesture, Drone };

struct AudioState {
    VoiceEngine* engine = nullptr;
    GesturePlayer* player = nullptr;
    std::atomic<int> mode{static_cast<int>(AppMode::Gesture)};
};

void audio_callback(ma_device* device, void* output, const void* /*input*/, ma_uint32 frame_count) {
    auto* state = static_cast<AudioState*>(device->pUserData);
    auto* out = static_cast<float*>(output);
    const int n = static_cast<int>(frame_count);
    if (state->mode.load(std::memory_order_acquire) == static_cast<int>(AppMode::Drone)) {
        state->engine->render(out, n);
        return;
    }
    for (int i = 0; i < n; ++i) {
        out[i] = state->engine->tick(state->player->next_params());
    }
}

bool write_wav_s16(const char* path, const float* samples, int n, int sample_rate) {
    std::vector<std::int16_t> pcm(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        float x = std::clamp(samples[i], -1.f, 1.f);
        pcm[static_cast<std::size_t>(i)] = static_cast<std::int16_t>(x * 32767.f);
    }

    const int data_bytes = n * 2;
    const int byte_rate = sample_rate * 2;
    unsigned char header[44]{};
    std::memcpy(header + 0, "RIFF", 4);
    const std::uint32_t riff_size = static_cast<std::uint32_t>(36 + data_bytes);
    std::memcpy(header + 4, &riff_size, 4);
    std::memcpy(header + 8, "WAVE", 4);
    std::memcpy(header + 12, "fmt ", 4);
    const std::uint32_t fmt_size = 16;
    std::memcpy(header + 16, &fmt_size, 4);
    const std::uint16_t audio_format = 1;
    const std::uint16_t channels = 1;
    const std::uint16_t block_align = 2;
    const std::uint16_t bits = 16;
    const std::uint32_t sr = static_cast<std::uint32_t>(sample_rate);
    const std::uint32_t br = static_cast<std::uint32_t>(byte_rate);
    std::memcpy(header + 20, &audio_format, 2);
    std::memcpy(header + 22, &channels, 2);
    std::memcpy(header + 24, &sr, 4);
    std::memcpy(header + 28, &br, 4);
    std::memcpy(header + 32, &block_align, 2);
    std::memcpy(header + 34, &bits, 2);
    std::memcpy(header + 36, "data", 4);
    const std::uint32_t ds = static_cast<std::uint32_t>(data_bytes);
    std::memcpy(header + 40, &ds, 4);

    std::ofstream out(path, std::ios::binary);
    if (!out) {
        return false;
    }
    out.write(reinterpret_cast<const char*>(header), 44);
    out.write(reinterpret_cast<const char*>(pcm.data()), data_bytes);
    return static_cast<bool>(out);
}

void render_player(VoiceEngine& engine, GesturePlayer& player, float* out, int n) {
    for (int i = 0; i < n; ++i) {
        out[i] = engine.tick(player.next_params());
    }
}

struct Stats {
    int bad = 0;
    float peak = 0.f;
    float rms = 0.f;
    float tail_rms = 0.f;
};

Stats measure(const float* x, int n, int tail) {
    Stats s;
    double sum_sq = 0.0;
    double tail_sq = 0.0;
    const int tail_n = std::min(tail, n);
    const int tail_start = n - tail_n;
    for (int i = 0; i < n; ++i) {
        if (!std::isfinite(x[i])) {
            ++s.bad;
            continue;
        }
        const float a = std::abs(x[i]);
        s.peak = std::max(s.peak, a);
        const double sq = static_cast<double>(x[i]) * static_cast<double>(x[i]);
        sum_sq += sq;
        if (i >= tail_start) {
            tail_sq += sq;
        }
    }
    s.rms = static_cast<float>(std::sqrt(sum_sq / n));
    s.tail_rms = (tail_n > 0) ? static_cast<float>(std::sqrt(tail_sq / tail_n)) : 0.f;
    return s;
}

int run_check() {
    const int n = kSampleRate;
    const int tail = kSampleRate / 20;
    std::vector<float> a(static_cast<std::size_t>(n));
    std::vector<float> b(static_cast<std::size_t>(n));

    VoiceEngine engine(static_cast<float>(kSampleRate));
    GesturePlayer player(static_cast<float>(kSampleRate));

    player.trigger_kind(GestureKind::Query);
    render_player(engine, player, a.data(), n);
    player.trigger_kind(GestureKind::Query);
    render_player(engine, player, b.data(), n);

    const Stats sa = measure(a.data(), n, tail);
    const Stats sb = measure(b.data(), n, tail);
    std::printf("query1: nan/inf=%d peak=%.4f rms=%.4f tail_rms=%.4f\n", sa.bad, sa.peak, sa.rms, sa.tail_rms);
    std::printf("query2: nan/inf=%d peak=%.4f rms=%.4f tail_rms=%.4f\n", sb.bad, sb.peak, sb.rms, sb.tail_rms);

    float max_diff = 0.f;
    for (int i = 0; i < n; ++i) {
        max_diff = std::max(max_diff, std::abs(a[static_cast<std::size_t>(i)] - b[static_cast<std::size_t>(i)]));
    }
    std::printf("query_diff_peak=%.4f\n", max_diff);

    if (!write_wav_s16("check.wav", a.data(), n, kSampleRate)) {
        std::fprintf(stderr, "failed to write check.wav\n");
        return 1;
    }
    std::printf("wrote check.wav\n");

    {
        VoiceEngine eng(static_cast<float>(kSampleRate));
        GesturePlayer pl(static_cast<float>(kSampleRate));
        pl.trigger_kind(GestureKind::Sad);
        render_player(eng, pl, b.data(), n);
        write_wav_s16("check_sad.wav", b.data(), n, kSampleRate);
        const Emotion speak{};
        pl.trigger_emotion(speak);
        render_player(eng, pl, b.data(), n);
        write_wav_s16("check_speak.wav", b.data(), n, kSampleRate);
        pl.trigger_kind(GestureKind::Coo);
        render_player(eng, pl, b.data(), n);
        write_wav_s16("check_coo.wav", b.data(), n, kSampleRate);
        pl.trigger_kind(GestureKind::Whine);
        render_player(eng, pl, b.data(), n);
        write_wav_s16("check_whine.wav", b.data(), n, kSampleRate);
        pl.trigger_kind(GestureKind::Excited);
        render_player(eng, pl, b.data(), n);
        write_wav_s16("check_excited.wav", b.data(), n, kSampleRate);
        std::printf("wrote check_sad.wav check_speak.wav check_coo.wav check_whine.wav check_excited.wav\n");
    }

    if (sa.bad > 0 || sb.bad > 0 || sa.peak < 0.02f || sb.peak < 0.02f || sa.peak > 0.98f ||
        sb.peak > 0.98f) {
        std::fprintf(stderr, "check failed: level or non-finite samples\n");
        return 1;
    }
    if (sa.tail_rms > 0.02f || sb.tail_rms > 0.02f) {
        std::fprintf(stderr, "check failed: utterance did not return to silence\n");
        return 1;
    }
    if (max_diff < 1.0e-3f) {
        std::fprintf(stderr, "check failed: two Query renders were too similar\n");
        return 1;
    }

    VoiceEngine drone(static_cast<float>(kSampleRate));
    VoiceParams dp;
    dp.amplitude = 1.f;
    drone.set_params(dp);
    drone.set_playing(true);
    drone.render(a.data(), n);
    const Stats sd = measure(a.data(), n, tail);
    std::printf("drone: nan/inf=%d peak=%.4f rms=%.4f\n", sd.bad, sd.peak, sd.rms);
    if (sd.bad > 0 || sd.peak < 0.02f || sd.peak > 0.98f) {
        std::fprintf(stderr, "check failed: drone path\n");
        return 1;
    }
    return 0;
}
}  // namespace

int main(int argc, char** argv) {
    if (argc > 1 && std::strcmp(argv[1], "--check") == 0) {
        return run_check();
    }

    if (!glfwInit()) {
        std::fprintf(stderr, "glfwInit failed\n");
        return 1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(560, 640, "Creature Voice", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "glfwCreateWindow failed\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    {
        ImGuiIO& io = ImGui::GetIO();
        ImFontConfig font_cfg;
        font_cfg.OversampleH = 1;
        font_cfg.OversampleV = 1;
        font_cfg.PixelSnapH = true;
        const char* font_paths[] = {
            "C:\\Windows\\Fonts\\meiryo.ttc",
            "C:\\Windows\\Fonts\\YuGothM.ttc",
            "C:\\Windows\\Fonts\\msgothic.ttc",
        };
        bool font_ok = false;
        for (const char* path : font_paths) {
            if (io.Fonts->AddFontFromFileTTF(path, 18.0f, &font_cfg, io.Fonts->GetGlyphRangesJapanese())) {
                font_ok = true;
                break;
            }
        }
        if (!font_ok) {
            io.Fonts->AddFontDefault();
        }
    }
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    VoiceEngine engine(static_cast<float>(kSampleRate));
    GesturePlayer player(static_cast<float>(kSampleRate));
    VoiceParams drone_params;
    drone_params.amplitude = 1.f;
    Emotion emotion;
    bool drone_mode = false;
    bool playing = true;
    engine.set_playing(playing);
    engine.set_params(drone_params);

    AudioState audio;
    audio.engine = &engine;
    audio.player = &player;
    audio.mode.store(static_cast<int>(AppMode::Gesture));

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;
    config.playback.channels = 1;
    config.sampleRate = kSampleRate;
    config.dataCallback = audio_callback;
    config.pUserData = &audio;
    config.periodSizeInFrames = 256;

    ma_device device;
    if (ma_device_init(nullptr, &config, &device) != MA_SUCCESS) {
        std::fprintf(stderr, "ma_device_init failed\n");
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    if (ma_device_start(&device) != MA_SUCCESS) {
        std::fprintf(stderr, "ma_device_start failed\n");
        ma_device_uninit(&device);
        return 1;
    }

    std::string status;
    const GestureKind kinds[] = {
        GestureKind::Query, GestureKind::Content, GestureKind::Call, GestureKind::Coo,
        GestureKind::Sad,   GestureKind::Excited, GestureKind::Chirp, GestureKind::Whine,
    };

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize, ImGuiCond_Always);
        ImGui::Begin("Creature Voice", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);

        ImGui::TextUnformatted("Emotion / 感情");
        ImGui::SliderFloat("Valence / 快不快", &emotion.valence, -1.f, 1.f, "%.2f");
        ImGui::SliderFloat("Arousal / 覚醒", &emotion.arousal, 0.f, 1.f, "%.2f");
        ImGui::SliderFloat("Affection / 愛情", &emotion.affection, 0.f, 1.f, "%.2f");
        ImGui::SliderFloat("Curiosity / 好奇心", &emotion.curiosity, 0.f, 1.f, "%.2f");
        ImGui::SliderFloat("Fear / 恐れ", &emotion.fear, 0.f, 1.f, "%.2f");

        const MappedGesture mapped = map_emotion(emotion);
        ImGui::Text("Speak as / 発声予定: %s", gesture_kind_label(mapped.kind));
        if (ImGui::Button("Speak / 話す", ImVec2(-1.f, 0.f))) {
            audio.mode.store(static_cast<int>(AppMode::Gesture), std::memory_order_release);
            drone_mode = false;
            player.trigger_emotion(emotion);
            status = std::string("spoke / 発声: ") + gesture_kind_label(mapped.kind);
        }

        ImGui::Separator();
        ImGui::TextUnformatted("Gestures / 発声ジェスチャー");
        for (int i = 0; i < 8; ++i) {
            if (i % 4 != 0) {
                ImGui::SameLine();
            }
            if (ImGui::Button(gesture_kind_label(kinds[i]), ImVec2(120.f, 0.f))) {
                audio.mode.store(static_cast<int>(AppMode::Gesture), std::memory_order_release);
                drone_mode = false;
                player.trigger_kind(kinds[i]);
                status = std::string("played / 再生: ") + gesture_kind_label(kinds[i]);
            }
        }

        if (ImGui::Button("Write utterance WAV / WAV書き出し")) {
            VoiceEngine offline(static_cast<float>(kSampleRate));
            GesturePlayer offline_player(static_cast<float>(kSampleRate));
            offline_player.trigger_emotion(emotion);
            std::vector<float> buf(static_cast<std::size_t>(kSampleRate));
            render_player(offline, offline_player, buf.data(), kSampleRate);
            if (write_wav_s16("creature_voice.wav", buf.data(), kSampleRate, kSampleRate)) {
                status = "wrote / 書き出し: creature_voice.wav";
            } else {
                status = "failed to write wav / 書き出し失敗";
            }
        }

        if (ImGui::CollapsingHeader("Drone / 連続音")) {
            if (ImGui::Checkbox("Continuous tone / 連続再生", &drone_mode)) {
                audio.mode.store(static_cast<int>(drone_mode ? AppMode::Drone : AppMode::Gesture),
                                 std::memory_order_release);
                if (drone_mode) {
                    drone_params.amplitude = 1.f;
                    engine.set_params(drone_params);
                    engine.set_playing(playing);
                }
            }
            if (ImGui::Checkbox("Playing / 再生中", &playing)) {
                engine.set_playing(playing);
            }
            ImGui::BeginDisabled(!drone_mode);
            ImGui::SliderFloat("Pitch / 高さ", &drone_params.pitch_hz, 180.f, 1100.f, "%.0f Hz");
            ImGui::SliderFloat("Breath / 息", &drone_params.breath, 0.f, 1.f, "%.2f");
            ImGui::SliderFloat("Mouth / 口", &drone_params.mouth, 0.f, 1.f, "%.2f");
            ImGui::SliderFloat("Jitter / 揺らぎ", &drone_params.jitter, 0.f, 1.f, "%.2f");
            ImGui::SliderFloat("Open / 声門開度", &drone_params.open_quotient, 0.45f, 0.88f, "%.2f");
            ImGui::SliderFloat("Nasal / 鼻腔", &drone_params.nasality, 0.f, 1.f, "%.2f");
            ImGui::SliderFloat("Rough / 粗さ", &drone_params.roughness, 0.f, 1.f, "%.2f");
            ImGui::EndDisabled();
            if (drone_mode) {
                drone_params.amplitude = 1.f;
                engine.set_params(drone_params);
            }
        }

        if (!status.empty()) {
            ImGui::TextUnformatted(status.c_str());
        }

        ImGui::End();
        ImGui::Render();

        int w = 0;
        int h = 0;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.08f, 0.08f, 0.10f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ma_device_uninit(&device);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
