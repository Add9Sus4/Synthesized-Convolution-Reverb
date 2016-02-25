/*
 ** convolve.h
 **
 ** Header file for convolve.c
 **
 */

#ifndef _CONVOLVE_H_
#define _CONVOLVE_H_

typedef struct complex {float Re; float Im;} complex;

complex complex_mult(complex a, complex b);

void fft(complex *v, int n, complex *tmp);

void ifft(complex *v, int n, complex *tmp);

int convolve(float *x, float *h, int lenX, int lenH, float **output);

#endif
