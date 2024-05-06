#include "detection.h"
#include <stdio.h>
#include <math.h>

#define MIN_FREQ 3 // Min frequency of desired tremor
#define MAX_FREQ 6 // Max frequency of desired tremor
#define INTENSITY_SCALING_FACTOR 10

/*
power = Power of signal
size = Size of power array
sampleRate = Sampling rate of the original signal

Returns an intensity value based on the max magnitude of a peak in a frequency range
*/
int detectPeakIntensity(float *power, int size, int sampleRate)
{
    float maxMag = 0.0;
    int peakIndex = -1;

    // Corresponding indices for desired frequencies
    int minI = floor((MIN_FREQ * size) / (float)sampleRate);
    int maxI = ceil((MIN_FREQ * size) / (float)sampleRate);

    // Finding max magnitude in the desired frequency range
    for (int i = minI; i <= maxI; i++)
    {
        if (power[i] > maxMag)
        {
            maxMag = power[i];
            peakIndex = i;
        }
    }

    if (peakIndex == -1)
    {
        return 0;
    }

    return (int)(maxMag);
}
