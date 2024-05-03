#include <math.h>
#include <stdio.h>

#include "dft.h"

#define PI 3.1415926535897932385

/*
x = The input signal
size = Number of points in the signal
N = Number of dft points
power = Power spectrum of DFT
*/
void dft(float *x, int size, int N, float *power)
{
    float Xreal, Ximag;
    for (int k = 0; k < N; k++)
    {
        Xreal = 0.0;
        Ximag = 0.0;

        for (int n = 0; n < size; n++)
        {
            Xreal += x[n] * cos(2 * PI * k * n / N);
            Ximag -= x[n] * sin(2 * PI * k * n / N);
        }

        power[k] = sqrt((Xreal * Xreal) + (Ximag * Ximag));
    }
}

// /*
// Xreal = The real portion of the dft
// Ximag = The imaginary portion of the dft
// N = Number of dft points
// psd = Power Spectral Density
// */
// void psd(float *Xreal, float *Ximag, int N, float *psd)
// {
//     for (int k = 0; k < N; k++)
//     {
//         psd[k] = (Xreal[k] * Xreal[k] + Ximag[k] * Ximag[k]) / N;
//     }
// }
