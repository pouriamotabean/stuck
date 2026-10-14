# STUCK
One-knob tempo-synced stutter VST3 by Pouria Motabean.

- Knob 0 = OFF (clean pass-through)
- Knob up = audio gets stuck and repeats: 1–25% → 1/8, 26–50% → 1/16, 51–75% → 1/32, 76–100% → 1/64 (follows host tempo)
- Automate the "Stutter" parameter in Cubase. Double-click the knob to reset to OFF.

Build: push to GitHub → Actions builds the Windows VST3 → download the artifact and copy `STUCK.vst3` to `C:\Program Files\Common Files\VST3`.
