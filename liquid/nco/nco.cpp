#include "nco.h"
#include <cmath>

namespace liquid {

const float Nco::PI = 3.14159265358979323846f;

Nco::Nco(NcoType type) 
    : m_type(type), m_dtheta(0.0f), m_theta(0.0f) {
}

Nco::~Nco() {
}

void Nco::set_frequency(float dtheta) {
    m_dtheta = dtheta;
}

void Nco::set_phase(float theta) {
    m_theta = theta;
}

void Nco::execute(float& s) {
    s = std::sin(m_theta);
    m_theta += m_dtheta;
    if (m_theta > PI) m_theta -= 2.0f * PI;
    if (m_theta < -PI) m_theta += 2.0f * PI;
}

void Nco::execute(std::complex<float>& c) {
    c = std::complex<float>(std::cos(m_theta), std::sin(m_theta));
    m_theta += m_dtheta;
    if (m_theta > PI) m_theta -= 2.0f * PI;
    if (m_theta < -PI) m_theta += 2.0f * PI;
}

void Nco::execute_block(std::complex<float>* c, unsigned int n) {
    for (unsigned int i = 0; i < n; i++) {
        execute(c[i]);
    }
}

void Nco::mix(float x, float& y) {
    float s;
    execute(s);
    y = x * s;
}

void Nco::mix(std::complex<float> x, std::complex<float>& y) {
    std::complex<float> nco_out;
    execute(nco_out);
    y = x * std::conj(nco_out);
}

void Nco::mix_block(std::complex<float>* x, unsigned int n) {
    for (unsigned int i = 0; i < n; i++) {
        std::complex<float> nco_out;
        execute(nco_out);
        x[i] = x[i] * std::conj(nco_out);
    }
}

} // namespace liquid
