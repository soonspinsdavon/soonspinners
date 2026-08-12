#include "VariSpinEngine.h"

void VariSpinEngine::prepare (double newSampleRate, int /*maxBlockSize*/, int numChannels)
{
    sampleRate = newSampleRate;

    // 2 seconds of headroom easily covers our 0.25x-4x speed range without
    // the read head catching up to (or being lapped by) the write head
    // under normal use.
    bufferLength = juce::jmax (1024, (int) (sampleRate * 2.0));
    delayBuffer.setSize (juce::jmax (1, numChannels), bufferLength);
    delayBuffer.clear();

    writePos = 0;
    readPos  = (double) (bufferLength / 2); // start trailing the write pointer

    smoothedRatio.reset (sampleRate, glideTimeMs / 1000.0);
    smoothedRatio.setCurrentAndTargetValue (baseSpeed);
}

void VariSpinEngine::reset()
{
    delayBuffer.clear();
    writePos = 0;
    readPos  = (double) (bufferLength / 2);
    wowPhase = 0.0;
    flutterPhase = 0.0;
}

void VariSpinEngine::setGlideTimeMs (float ms)
{
    glideTimeMs = juce::jmax (1.0f, ms);
    smoothedRatio.reset (sampleRate, glideTimeMs / 1000.0);
}

void VariSpinEngine::setSpinDown (bool isHeld, float targetRatio) noexcept
{
    spinDownHeld  = isHeld;
    spinDownRatio = targetRatio;
}

void VariSpinEngine::setSpinUp (bool isHeld, float targetRatio) noexcept
{
    spinUpHeld  = isHeld;
    spinUpRatio = targetRatio;
}

float VariSpinEngine::computeTargetRatio() const noexcept
{
    // Spin buttons override the manual speed knob while held, and it
    // glides back to the manual speed the moment they're released -
    // same interaction model as Vari-Fi's momentary spin.
    if (spinDownHeld) return spinDownRatio;
    if (spinUpHeld)   return spinUpRatio;
    return baseSpeed;
}

void VariSpinEngine::process (juce::AudioBuffer<float>& buffer)
{
    if (bufferLength == 0)
        return;

    const int numChannels   = buffer.getNumChannels();
    const int numSamples    = buffer.getNumSamples();
    const int delayChannels = delayBuffer.getNumChannels();

    smoothedRatio.setTargetValue (computeTargetRatio());

    const double wowIncrement     = juce::MathConstants<double>::twoPi * wowRateHz / sampleRate;
    const double flutterIncrement = juce::MathConstants<double>::twoPi * flutterRateHz / sampleRate;

    for (int i = 0; i < numSamples; ++i)
    {
        const float smoothedBase = smoothedRatio.getNextValue();

        // Wow/flutter ride on top of the smoothed base rate rather than
        // being smoothed themselves - they're supposed to be a constant
        // wobble, not something that glides.
        const float wow     = (float) std::sin (wowPhase)     * wowMaxDepth     * wobbleAmount;
        const float flutter = (float) std::sin (flutterPhase) * flutterMaxDepth * wobbleAmount;
        const float ratio   = smoothedBase * (1.0f + wow + flutter);

        wowPhase += wowIncrement;
        if (wowPhase >= juce::MathConstants<double>::twoPi)
            wowPhase -= juce::MathConstants<double>::twoPi;

        flutterPhase += flutterIncrement;
        if (flutterPhase >= juce::MathConstants<double>::twoPi)
            flutterPhase -= juce::MathConstants<double>::twoPi;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* channelData = buffer.getWritePointer (ch);
            auto* delayData   = delayBuffer.getWritePointer (juce::jmin (ch, delayChannels - 1));

            const float drySample = channelData[i];

            delayData[writePos] = drySample;

            const int readIndex0 = (int) readPos;
            const int readIndex1 = (readIndex0 + 1) % bufferLength;
            const float frac     = (float) (readPos - (double) readIndex0);

            const float s0 = delayData[readIndex0];
            const float s1 = delayData[readIndex1];
            const float wetSample = s0 + frac * (s1 - s0);

            channelData[i] = drySample + mix * (wetSample - drySample);
        }

        writePos = (writePos + 1) % bufferLength;
        readPos += (double) ratio;

        if (readPos >= (double) bufferLength)
            readPos -= (double) bufferLength;

        // Cheap anti-collision safeguard: if the read head strays too
        // close to (or too far behind) the write head, snap it back to
        // the middle of the buffer. TODO: replace with a crossfaded
        // dual-read-head to make this glitch-free.
        double distanceBehindWrite = (double) writePos - readPos;
        if (distanceBehindWrite < 0.0)
            distanceBehindWrite += bufferLength;

        const double minHeadroom = 64.0;
        const double maxHeadroom = (double) bufferLength - 64.0;
        if (distanceBehindWrite < minHeadroom || distanceBehindWrite > maxHeadroom)
        {
            readPos = (double) writePos - (double) (bufferLength / 2);
            if (readPos < 0.0)
                readPos += bufferLength;
        }
    }
}
