#include "agc.h"
#include <cmath>

namespace liquid {

Agc::Agc(float bt, float beta, float signal_level) 
    : m_bt(bt), m_beta(beta), m_signal_level(signal_level), 
      m_rssi(0.0f), m_alpha(bt) {
}

Agc::~Agc() {
}

void Agc::execute(std::complex<float>& x) {
    // Estimate RSSI from input signal
    float mag = std::sqrt(x.real() * x.real() + x.imag() * x.imag());
    m_rssi = m_alpha * mag + (1.0f - m_alpha) * m_rssi;

    // Apply gain based on current RSSI estimate
    float gain = m_signal_level / (m_rssi + 1e-10f);
    x *= gain;
}

void Agc::execute(float& x) {
    float mag = std::fabs(x);
    m_rssi = m_alpha * mag + (1.0f - m_alpha) * m_rssi;
    float gain = m_signal_level / (m_rssi + 1e-10f);
    x *= gain;
}

void Agc::execute_block(std::complex<float>* x, unsigned int n) {
    for (unsigned int i = 0; i < n; i++) {
        execute(x[i]);
    }
}

void Agc::set_bandwidth(float bt) {
    m_bt = bt;
    m_alpha = bt;
}

void Agc::set_signal_level(float level) {
    m_signal_level = level;
}

void Agc::reset() {
    m_rssi = 0.0f;
}

} // namespace liquid
