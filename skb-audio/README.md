# SKB Audio Lab v1.1 — experimental firmware

Standalone M5StickC Plus2 microphone-to-phone audio lab. This project lives in `skb-audio/` and does not modify Mambo OS on `main`.

## Features in source
- Password-protected Wi-Fi AP `SKB-Audio`; default password `SKBAudio2026!` (change before real use)
- Browser interface at http://192.168.4.1
- Live listen using repeated PCM HTTP blocks, live waveform and browser Web Audio spectrum (0–8 kHz)
- Tap the spectrum to choose a centre frequency; independently adjustable Band A and B peaking EQ (frequency, gain, bandwidth)
- Auto Tune selected frequency; Auto Track strongest peak; 10-band EQ, presets, bypass, low-cut, playback volume, digital input gain
- Visible record and stop/save controls; WAV downloads to phone; raw PCM or browser-rendered full-EQ capture
- Battery status and device reboot

## Build
GitHub Actions: **Build SKB Audio Recorder**, branch `skb-audio-recorder`.
The workflow builds with PlatformIO and uploads firmware artifacts if compilation succeeds.

## Important limitations
- The source was committed, but this revision has **not yet been confirmed to compile or run on hardware**.
- Recording is limited to 3 minutes to reduce memory pressure. The browser audio path uses repeated HTTP fetches. Audio gaps and latency are possible; continuous real-time streaming is not yet proven.
- Spectrum analysis depends on browser playback and may show silence while muted.
- Processed recording export uses browser OfflineAudioContext to render the gain, low-cut, 10-band EQ and both AutoTune bands at the settings active when recording stops. It may take time to render.
- Auto Track follows the strongest spectral peak, which may be noise, and can jump rapidly.
- The waveform is sourced from microphone PCM. The spectrum comes from the browser analyser.
- The SKB lettering is CSS text, not the original uploaded logo image.
- Sample rate is fixed at 16 kHz mono. Adjustable quality and noise reduction beyond low-cut remain future work.
- There is no local recording library or persistent device storage.
- Browser permissions and Android download behaviour must be tested on the phone.
- Recordings and listening should be initiated with everyone involved aware of and consenting to capture.

Do not flash until the new workflow build passes and device compatibility has been reviewed.

## Pre-release phone + hardware test checklist

1. Build the latest branch and verify the workflow succeeds. Do not flash only because an earlier revision passed.
2. Back up your current firmware before flashing. Verify board, flash size and firmware artifact/offsets.
3. Boot the M5StickC Plus2 and connect Samsung S21 to Wi-Fi `SKB-Audio` (password `SKBAudio2026!`); stay connected even if Android says no Internet.
4. Open `http://192.168.4.1` in Chrome. Confirm SKB branding, battery, and stream diagnostics.
5. Press **DOWNLOAD 2-SECOND TEST TONE WAV**; confirm a playable 440 Hz tone. This tests browser download only.
6. Tap **LIVE LISTEN**. Check sound through the phone, waveform movement, peak frequency and diagnostic errors. Avoid acoustic feedback by keeping phone output away from the M5 microphone.
7. Tap distinct spectrum peaks, select Band A and Band B, press AutoTune, and verify that each band's frequency and boost/cut changes the heard sound.
8. Test 10-band EQ presets, bypass, gain, low-cut and volume. Compare original and filtered sound.
9. Record 10 seconds in Original mode, stop/save and play the downloaded WAV. Repeat in Full EQ processed mode and compare.
10. Test Mute, Stop Listen, restart listening, reconnect Wi-Fi, and device reboot.
11. Test a longer recording only after short recordings work. Maximum duration is 180 seconds.
12. Record any problems with the firmware commit, Android Chrome version, and stream diagnostics.

## Diagnostics
The UI reports received PCM blocks, HTTP stream errors, estimated capture throughput and recorded seconds. It also includes a phone-generated 440 Hz WAV download self-test.

## Known issues to assess
- HTTP chunk polling is not a true persistent low-latency audio transport. Under some conditions it may stutter or lag.
- Microphone sample capture and browser audio output have not yet been verified together on physical M5StickC Plus2 hardware.
- Audio filters and FFT are processed in the phone browser, not in ESP32 firmware.
- The browser records audio in memory until download; do not close Chrome during recording.
- The original uploaded SKB logo is not embedded; the UI currently renders SKB as styled text.
- Auto Track can follow unwanted noise, especially with low input levels.
- No automatic gain control, advanced noise suppression or adjustable sample rate is implemented.

## Experimental streaming and adaptive audio update
- Persistent raw PCM HTTP stream on TCP port 81 (`http://192.168.4.1:81/live`) replaces repeated short polling in the browser. Port 80 still serves the UI and status.
- Optional adaptive noise gate uses measured RMS noise floor; this is **not** neural or spectral noise suppression. It can cut quiet speech, and needs real-device tuning.
- Optional automatic gain control uses a smoothed RMS-based gain estimate; it may raise background noise and can clip.
- Low-cut filtering and the existing ten-band EQ remain available.
- Browser recording in Original mode stores unprocessed PCM. Processed WAV export applies the low-cut and EQ but currently **does not include** the adaptive gate or automatic gain changes used during live playback.
- The persistent stream and new audio conditioning have not been verified on hardware; build success alone does not prove real-time playback quality.
- The original SKB image asset was not yet embedded; the UI still displays CSS lettering. An exact original image must be recovered and converted to a firmware-friendly embedded asset before claiming this is done.

## Logo build verification
The exact uploaded JPEG at `assets/SKB_Audio_Logo.jpg` is embedded at build time. The build must pass before flashing; the UI falls back to text only if the asset is missing.
