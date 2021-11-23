#ifndef PLOT_H
#define PLOT_H

#include <complex.h>
#include <stddef.h>

void plot_amplitude_spectrum(float* ampl, size_t len);

void do_plotting(float complex* samples, size_t num_samples);

#endif //PLOT_H
