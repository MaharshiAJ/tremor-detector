#include <math.h>
#include <stdio.h>

#define PI 3.1415926535897932385

/*
x = The input signal
size = Number of points in the signal
N = Number of dft points
Xreal = The real portion of the dft
Ximag = The imaginary portion of the dft
*/
void dft(float *x, int size, int N, float *Xreal, float *Ximag)
{
    for (int k = 0; k < N; k++)
    {
        Xreal[k] = 0;
        Ximag[k] = 0;
        for (int n = 0; n < size; n++)
        {
            Xreal[k] += x[n] * cos(2 * PI * k * n / N);
            Ximag[k] -= x[n] * sin(2 * PI * k * n / N);
        }
    }
}

/*
Xreal = The real portion of the dft
Ximag = The imaginary portion of the dft
N = Number of dft points
psd = Power Spectral Density
*/
void psd(float *Xreal, float *Ximag, int N, float *psd)
{
    for (int k = 0; k < N; k++)
    {
        psd[k] = (Xreal[k] * Xreal[k] + Ximag[k] * Ximag[k]) / N;
    }
}