#pragma once
// STUCK – one-knob tempo-synced stutter engine (no JUCE dependency, testable on its own)
//
// Knob 0      -> OFF, clean pass-through.
// Knob > 0    -> audio "gets stuck": the moment the knob leaves zero the engine marks a
//                start point and loops the next N ms (live on the first pass, then repeats).
// Higher knob -> shorter, tempo-synced slices (1/8 -> 1/16 -> 1/32 -> 1/64),
//                so automating the knob up makes the stutter faster / more glitchy.
// Back to 0   -> smooth return to the dry signal.

#include <vector>
#include <cmath>
#include <algorithm>
#include <atomic>

class StutterEngine
{
public:
    static constexpr int maxChannels = 2;

    void prepare (double sampleRate, int /*maxBlock*/)
    {
        sr = sampleRate;
        bufLen = (int) std::ceil (sr * 6.0);           // 6 s history, plenty for 1/8 at 30 BPM
        for (auto& b : buf) b.assign ((size_t) bufLen, 0.0f);
        reset();
    }

    void reset()
    {
        for (auto& b : buf) std::fill (b.begin(), b.end(), 0.0f);
        writePos = 0;
        engaged = false;
        wetGain = 0.0f;
        readPos = 0;
        jumpRemain = 0;
        currentLoopMs.store (0.0f);
    }

    // Division (in quarter-notes) for a knob value 0..1. 0 means off.
    static double divisionForKnob (float knob)
    {
        if (knob <= 0.0005f) return 0.0;
        if (knob <= 0.25f)   return 0.5;     // 1/8
        if (knob <= 0.50f)   return 0.25;    // 1/16
        if (knob <= 0.75f)   return 0.125;   // 1/32
        return 0.0625;                        // 1/64
    }

    // knobStart / knobEnd: parameter value at start and end of block (for sample-accurate ramps)
    void process (float* const* ch, int numCh, int n, float knobStart, float knobEnd, double bpm)
    {
        numCh = std::min (numCh, maxChannels);
        if (bpm <= 1.0 || ! std::isfinite (bpm)) bpm = 120.0;

        const float fadeStep = 1.0f / (float) std::max (1.0, sr * 0.004);   // 4 ms wet/dry fades
        const int   xfLen    = std::max (8, (int) (sr * 0.003));            // 3 ms loop-seam crossfade
        const int   jumpLen  = std::max (8, (int) (sr * 0.004));            // 4 ms declick on length change

        for (int i = 0; i < n; ++i)
        {
            const float knob = knobStart + (knobEnd - knobStart) * ((float) i / (float) std::max (1, n));
            const double div = divisionForKnob (knob);

            // write input into history (but never overwrite a slice that is currently stuck)
            bool canWrite = true;
            if (engaged)
            {
                int d = (writePos - loopStart) % bufLen; if (d < 0) d += bufLen;
                const bool firstPass = elapsed < protLen;
                canWrite = firstPass || (d >= protLen && d < bufLen - xfLen - 2);
                if (elapsed < bufLen) ++elapsed;
            }
            if (canWrite)
            {
                for (int c = 0; c < numCh; ++c)
                    buf[(size_t) c][(size_t) writePos] = ch[c][i];
                if (numCh == 1) buf[1][(size_t) writePos] = ch[0][i];
            }

            if (div > 0.0)
            {
                const int newLen = std::max (16, (int) std::lround (sr * 60.0 / bpm * div));

                if (! engaged)
                {
                    engaged   = true;
                    loopStart = writePos;      // stick to "now"
                    loopLen   = newLen;
                    readPos   = 0;
                    jumpRemain = 0;
                    elapsed   = 1;
                    protLen   = std::min (bufLen / 2, (int) std::lround (sr * 60.0 / bpm * 0.5) + 1);
                }
                else if (newLen != loopLen)
                {
                    // shrink/grow the slice from the same start point, declick the jump
                    const int newRead = readPos % newLen;
                    jumpFrom    = readPos;      // keep playing the old slice briefly and fade it out
                    jumpFromLen = loopLen;
                    jumpRemain  = jumpLen;
                    loopLen = newLen;
                    readPos = newRead;
                }
                wetGain = std::min (1.0f, wetGain + fadeStep);
            }
            else
            {
                wetGain = std::max (0.0f, wetGain - fadeStep);
                if (wetGain <= 0.0f) engaged = false;
            }

            if (engaged || wetGain > 0.0f)
            {
                for (int c = 0; c < numCh; ++c)
                {
                    float wet = readLoop (c, readPos, loopLen, xfLen);
                    if (jumpRemain > 0)
                    {
                        const float t = (float) jumpRemain / (float) jumpLen;    // 1 -> 0
                        const float old = readLoop (c, jumpFrom, jumpFromLen, xfLen);
                        wet = wet * (1.0f - t) + old * t;
                    }
                    ch[c][i] = ch[c][i] * (1.0f - wetGain) + wet * wetGain;
                }
                if (jumpRemain > 0) { --jumpRemain; jumpFrom = (jumpFrom + 1) % std::max (1, jumpFromLen); }
                readPos = (readPos + 1) % std::max (1, loopLen);
            }

            writePos = (writePos + 1) % bufLen;
        }

        currentLoopMs.store (engaged ? (float) (1000.0 * loopLen / sr) : 0.0f);
    }

    std::atomic<float> currentLoopMs { 0.0f };

private:
    // sample p of the loop, crossfading the tail into the audio just before the start
    float readLoop (int c, int p, int L, int xfLen) const
    {
        const int xf = std::min (xfLen, L / 4);
        float v = at (c, loopStart + p);
        if (xf > 0 && p >= L - xf)
        {
            const float f = (float) (p - (L - xf) + 1) / (float) xf;     // 0 -> 1
            v = v * (1.0f - f) + at (c, loopStart + p - L) * f;
        }
        return v;
    }

    float at (int c, int idx) const
    {
        idx %= bufLen; if (idx < 0) idx += bufLen;
        return buf[(size_t) c][(size_t) idx];
    }

    double sr = 44100.0;
    std::vector<float> buf[maxChannels];
    int bufLen = 1, writePos = 0;
    bool engaged = false;
    float wetGain = 0.0f;
    int loopStart = 0, loopLen = 1, readPos = 0;
    int jumpFrom = 0, jumpFromLen = 1, jumpRemain = 0;
    int elapsed = 0, protLen = 1;
};
