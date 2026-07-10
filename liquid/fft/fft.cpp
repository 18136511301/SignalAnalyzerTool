#include "fft.h"
#include <algorithm>
#include <cstring>

namespace liquid {

FFT::FFT(unsigned int nfft, int direction) 
    : m_nfft(nfft), m_direction(direction) {
}

FFT::~FFT() {
}

void FFT::execute(std::complex<float>* data, std::complex<float>* output) {
    std::memcpy(output, data, m_nfft * sizeof(std::complex<float>));
    bit_reverse(output, m_nfft);
    
    for (unsigned int size = 2; size <= m_nfft; size *= 2) {
        unsigned int half = size / 2;
        float angle = m_direction * -2.0f * 3.14159265358979323846f / size;
        std::complex<float> w(std::cos(angle), std::sin(angle));
        
        for (unsigned int i = 0; i < m_nfft; i += size) {
            std::complex<float> twiddle(1.0f, 0.0f);
            for (unsigned int j = 0; j < half; j++) {
                std::complex<float> u = output[i + j];
                std::complex<float> t = twiddle * output[i + j + half];
                output[i + j] = u + t;
                output[i + j + half] = u - t;
                twiddle *= w;
            }
        }
    }
    
    if (m_direction == -1) {
        float scale = 1.0f / m_nfft;
        for (unsigned int i = 0; i < m_nfft; i++) {
            output[i] *= scale;
        }
    }
}

void FFT::execute(std::complex<double>* data, std::complex<double>* output) {
    std::memcpy(output, data, m_nfft * sizeof(std::complex<double>));
    bit_reverse(output, m_nfft);
    
    for (unsigned int size = 2; size <= m_nfft; size *= 2) {
        unsigned int half = size / 2;
        double angle = m_direction * -2.0 * 3.14159265358979323846 / size;
        std::complex<double> w(std::cos(angle), std::sin(angle));
        
        for (unsigned int i = 0; i < m_nfft; i += size) {
            std::complex<double> twiddle(1.0, 0.0);
            for (unsigned int j = 0; j < half; j++) {
                std::complex<double> u = output[i + j];
                std::complex<double> t = twiddle * output[i + j + half];
                output[i + j] = u + t;
                output[i + j + half] = u - t;
                twiddle *= w;
            }
        }
    }
    
    if (m_direction == -1) {
        double scale = 1.0 / m_nfft;
        for (unsigned int i = 0; i < m_nfft; i++) {
            output[i] *= scale;
        }
    }
}

void FFT::bit_reverse(std::complex<float>* data, unsigned int n) {
    unsigned int j = 0;
    for (unsigned int i = 0; i < n - 1; i++) {
        if (i < j) {
            std::swap(data[i], data[j]);
        }
        unsigned int k = n >> 1;
        while (k <= j) {
            j -= k;
            k >>= 1;
        }
        j += k;
    }
}

void FFT::bit_reverse(std::complex<double>* data, unsigned int n) {
    unsigned int j = 0;
    for (unsigned int i = 0; i < n - 1; i++) {
        if (i < j) {
            std::swap(data[i], data[j]);
        }
        unsigned int k = n >> 1;
        while (k <= j) {
            j -= k;
            k >>= 1;
        }
        j += k;
    }
}

} // namespace liquid
