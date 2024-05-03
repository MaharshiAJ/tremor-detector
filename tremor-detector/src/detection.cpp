#include "detection.h"
#include <stdio.h>

#define MIN_FREQ 3 // Min frequency of desired tremor
#define MAX_FREQ 6 // Max frequency of desired tremor

/*
mag = Magnitude of DFT of signal
size = Size of magnitude array
sampleRate = Sampling rate of the original signal

Returns an intensity value based on the max magnitude of a peak in a frequency range
*/
int detectPeakIntensity(float *mag, int size, int sampleRate)
{
    float maxMag = 0.0;

    // Corresponding indices for desired frequencies
    int minI = (MIN_FREQ * size) / sampleRate;
    int maxI = (MAX_FREQ * size) / sampleRate;

    // Finding max magnitude in the desired frequency range
    for (int i = minI; i <= maxI; i++)
    {
        printf("maxmag: %f\n", maxMag);
        if (mag[i] > maxMag)
        {
            maxMag = mag[i];
        }
    }

    int intensity = (int)(maxMag);

    return intensity;
}
