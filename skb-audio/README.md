# SKB Audio Recorder (experimental)
Standalone M5StickC Plus2 microphone-to-phone WAV recorder.
This lives under skb-audio/ and does not modify Mambo OS.

Build with GitHub Actions workflow **Build SKB Audio Recorder**.
Connect phone to SSID SKB-Audio, password SKBAudio2026!, then open http://192.168.4.1.
Tap RECORD WAV to download a 1–30 second recording to the phone.

**Limitations:** Initial hardware build unverified. Microphone capture and Android Chrome downloads require device testing. Stop cancels the transfer rather than preserving a partial recording. The waveform shown is illustrative, not a live microphone reading; it must not be treated as a sound-level measurement. No continuous monitoring or background recording. Recording is visibly initiated by the phone user. The default Wi-Fi password should be changed before use.

Do not flash until build passes and device-specific microphone pinout/behaviour is validated.
