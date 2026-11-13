# SKB Audio Lab v1.1 — experimental firmware

Standalone M5StickC Plus2 microphone-to-phone audio lab. This project lives in `skb-audio/` and does not modify Mambo OS on `main`.

## Features in source
- Password-protected Wi-Fi AP `SKB-Audio`; default password `SKBAudio2026!` (change before real use)
- Browser interface at http://192.168.4.1
- Live listen using repeated PCM HTTP blocks, live waveform and browser Web Audio spectrum (0–8 kHz)
- Tap the spectrum to choose a centre frequency; independently adjustable Band A and B peaking EQ (frequency, gain, bandwidth)
- Auto Tune selected frequency; Auto Track strongest peak; 10-band EQ, presets, bypass, low-cut, playback volume, digital input gain
- Visible record and stop/save controls; WAV downloads to phone; raw PCM or digital-gain-adjusted capture
- Battery status and device reboot

## Build
GitHub Actions: **Build SKB Audio Recorder**, branch `skb-audio-recorder`.
The workflow builds with PlatformIO and uploads firmware artifacts if compilation succeeds.

## Important limitations
- The source was committed, but this revision has **not yet been confirmed to compile or run on hardware**.
- The browser audio path uses repeated HTTP fetches. Audio gaps and latency are possible; continuous real-time streaming is not yet proven.
- Spectrum analysis depends on browser playback and may show silence while muted.
- The processed recording choice applies digital input gain, **not the full 10-band EQ or dual-band filter chain**. Filtered audio export still requires implementation.
- Auto Track follows the strongest spectral peak, which may be noise, and can jump rapidly.
- The waveform is sourced from microphone PCM. The spectrum comes from the browser analyser.
- The SKB lettering is CSS text, not the original uploaded logo image.
- Sample rate is fixed at 16 kHz mono. Adjustable quality and noise reduction beyond low-cut remain future work.
- There is no local recording library or persistent device storage.
- Browser permissions and Android download behaviour must be tested on the phone.
- Recordings and listening should be initiated with everyone involved aware of and consenting to capture.

Do not flash until the new workflow build passes and device compatibility has been reviewed.
