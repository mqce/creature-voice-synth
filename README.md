# Creature Voice Synth

小動物向けのノンバーバル鳴き声を、録音再生ではなくパラメトリック合成でリアルタイム生成する Windows アプリです。声門パルスとフォルマントで声を作り、感情／発声ジェスチャーから毎回少し違う声を出します。コアは OS 非依存で、将来 ESP32-S3 へ移植しやすい構成です。

## ビルド

CMake 3.20+、C++17、ネットワーク（初回に GLFW / Dear ImGui / miniaudio を取得）が必要です。

```powershell
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
.\build\creature_voice.exe
```

オフライン検査:

```powershell
.\build\creature_voice.exe --check
```

## 使い方

- **Speak** — 感情スライダーから種類を選んで 1 声出す
- **Gestures** — Query / Content / Call / Coo / Sad / Excited / Chirp / Whine
- **Drone** — 連続音。Pitch / Breath / Mouth / Jitter / Open / Nasal / Rough

同じボタンでも高さ・長さ・口が少しずつ変わります。発声後は無音に戻ります。
