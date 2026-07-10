#pragma once
#include "../core/types.h"
#include <vector>
#include <complex>
#include <cmath>

namespace liquid {

class FFT {
public:
    FFT(unsigned int nfft, int direction = 1);
    ~FFT();

    void execute(std::complex<float>* data, std::complex<float>* output);
    void execute(std::complex<double>* data, std::complex<double>* output);
    
    unsigned int size() const { return m_nfft; }
    int direction() const { return m_direction; }

private:
    unsigned int m_nfft;
    int m_direction;
    
    void fft_radix2(std::complex<float>* data, unsigned int n, int dir);
    void fft_radix2(std::complex<double>* data, unsigned int n, int dir);
    void bit_reverse(std::complex<float>* data, unsigned int n);
    void bit_reverse(std::complex<double>* data, unsigned int n);
};

} // namespace liquid
