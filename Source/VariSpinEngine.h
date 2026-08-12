#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

/**
    Core DSP for SoonSpinner.

    This is a variable-rate resampling engine, not a pitch-preserving
    time-stretcher: when playback speed changes, pitch moves with it,
    the same way slowing down a turntable or tape machine does. That
    coupling is what gives Vari-Fi-style plugins their character.

    How it works:
      - Incoming audio is continuously written into a circular buffer
        at a fixed rate (1 sample per sample, like normal recording).
      - A separate "read head" walks back through that buffer to
        produce the output, but its speed can be anything from a
        crawl to 4x. When the read head moves slower than the write
        head, you hear the audio slow down and drop in pitch; faster,
        and it speeds up and rises in pitch.
      - The read speed itself is smoothed ("glide") rather than
        snapping instantly, which mimics motor/tape inertia.

    Known limitation (intentional, for the MVP): if the read head
    drifts too far from the write head, it gets silently repositioned
    ("nudged"), which can cause an audible jump under extreme,
    sustained pitch shifts. A future pass can fix this properly with
    a second, crossfaded read head. Left as a TODO on purpose so we
    can tackle it together once the core sound is working.
*/
class VariSpinEngine
{
public:
    VariSpinEngine() = default;

    void prepare (double sampleRate, int maxBlockSize, int numChannels);
    void reset();

    /** Manual base speed ratio: 1.0 = normal, 0.5 = half speed (down an octave),
        2.0 = double speed (up an octave). Range is enforced by the plugin's
        parameter, not here. */
    void setBaseSpeed (float ratio) noexcept { baseSpeed = ratio; }

    /** How long (ms) it takes the read head to glide to a new target speed.
        Bigger = more "motor with mass" inertia; smaller = snappier. */
    void setGlideTimeMs (float ms);

    /** Call every block with the current held-state of the momentary
        spin controls and the ratio they should spin to. */
    void setSpinDown (bool isHeld, float targetRatio) noexcept;
    void setSpinUp   (bool isHeld, float targetRatio) noexcept;

    /** Dry/wet, 0 = fully dry, 1 = fully wet. */
    void setMix (float wetAmount) noexcept { mix = juce::jlimit (0.0f, 1.0f, wetAmount); }

    /** Master wow/flutter macro, 0 = off, 1 = maximum wobble. Internally this
        scales a slow "wow" LFO (~0.8Hz, motor-speed-style drift) and a
        faster "flutter" LFO (~6Hz, tape-contact-style flicker) that both
        multiply the read rate. Exposing them as separate rate/depth pairs
        is a natural follow-up once this sounds right. */
    void setWobbleAmount (float amount) noexcept { wobbleAmount = juce::jlimit (0.0f, 1.0f, amount); }

    void process (juce::AudioBuffer<float>& buffer);

private:
    float computeTargetRatio() const noexcept;

    double sampleRate = 44100.0;
    float baseSpeed   = 1.0f;
    float mix         = 1.0f;

    bool  spinDownHeld = false;
    bool  spinUpHeld   = false;
    float spinDownRatio = 0.5f;
    float spinUpRatio   = 2.0f;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> smoothedRatio;
    float glideTimeMs = 200.0f;

    // Wow/flutter state
    float wobbleAmount = 0.0f;
    static constexpr float wowRateHz     = 0.8f;
    static constexpr float wowMaxDepth   = 0.02f;  // +-2% speed at full wobble
    static constexpr float flutterRateHz = 6.0f;
    static constexpr float flutterMaxDepth = 0.008f; // +-0.8% speed at full wobble
    double wowPhase = 0.0;
    double flutterPhase = 0.0;

    juce::AudioBuffer<float> delayBuffer;
    int writePos      = 0;
    double readPos     = 0.0;
    int bufferLength   = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VariSpinEngine)
};
