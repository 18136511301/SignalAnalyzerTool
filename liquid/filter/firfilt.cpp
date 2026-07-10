#include "firfilt.h"

namespace liquid {

FirFilter::FirFilter(const std::vector<float>& taps)
    : m_taps(taps), m_buffer(taps.size(), std::complex<float>(0.0f, 0.0f)), m_index(0) {
}

FirFilter::FirFilter(const float* taps, unsigned int n)
    : m_taps(taps, taps + n), m_buffer(n, std::complex<float>(0.0f, 0.0f)), m_index(0) {
}

FirFilter::~FirFilter() {
}

void FirFilter::execute(const std::complex<float>& x, std::complex<float>& y) {
    m_buffer[m_index] = x;

    y = std::complex<float>(0.0f, 0.0f);
    unsigned int idx = m_index;
    for (unsigned int i = 0; i < m_taps.size(); i++) {
        y += m_taps[i] * m_buffer[idx];
        idx = (idx + m_taps.size() - 1) % m_taps.size();
    }

    m_index = (m_index + 1) % m_taps.size();
}

void FirFilter::execute_block(const std::complex<float>* x, unsigned int n, std::complex<float>* y) {
    for (unsigned int i = 0; i < n; i++) {
        execute(x[i], y[i]);
    }
}

void FirFilter::execute(float x, float& y) {
    m_buffer[m_index] = x;
    
    y = 0.0f;
    unsigned int idx = m_index;
    for (unsigned int i = 0; i < m_taps.size(); i++) {
        y += m_taps[i] * m_buffer[idx].real();
        idx = (idx + m_taps.size() - 1) % m_taps.size();
    }
    
    m_index = (m_index + 1) % m_taps.size();
}

void FirFilter::execute_block(const float* x, unsigned int n, float* y) {
    for (unsigned int i = 0; i < n; i++) {
        execute(x[i], y[i]);
    }
}

void FirFilter::reset() {
    std::fill(m_buffer.begin(), m_buffer.end(), std::complex<float>(0.0f, 0.0f));
    m_index = 0;
}

} // namespace liquid
