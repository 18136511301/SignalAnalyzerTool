#include "modem.h"
#include <cmath>
#include <algorithm>
#include <limits>

namespace liquid {

Modem::Modem(ModulationType type) : m_type(type), m_order(0) {
    init_constellation();
}

Modem::~Modem() {
}

void Modem::init_constellation() {
    switch (m_type) {
        case LIQUID_MODEM_BPSK:
            m_order = 2;
            m_constellation = {{1.0f, 0.0f}, {-1.0f, 0.0f}};
            break;
        case LIQUID_MODEM_QPSK:
            m_order = 4;
            m_constellation = {
                {0.7071f, 0.7071f}, {-0.7071f, 0.7071f},
                {-0.7071f, -0.7071f}, {0.7071f, -0.7071f}
            };
            break;
        case LIQUID_MODEM_8PSK:
            m_order = 8;
            for (unsigned int i = 0; i < 8; i++) {
                float angle = 2.0f * 3.14159265358979323846f * i / 8.0f;
                m_constellation.push_back({std::cos(angle), std::sin(angle)});
            }
            break;
        case LIQUID_MODEM_16QAM:
            m_order = 16;
            {
                float levels[] = {-3.0f, -1.0f, 1.0f, 3.0f};
                for (int i = 0; i < 4; i++) {
                    for (int q = 0; q < 4; q++) {
                        m_constellation.push_back({levels[i] / 3.1623f, levels[q] / 3.1623f});
                    }
                }
            }
            break;
        case LIQUID_MODEM_64QAM:
            m_order = 64;
            {
                float levels[] = {-7.0f, -5.0f, -3.0f, -1.0f, 1.0f, 3.0f, 5.0f, 7.0f};
                float norm = std::sqrt(42.0f);
                for (int i = 0; i < 8; i++) {
                    for (int q = 0; q < 8; q++) {
                        m_constellation.push_back({levels[i] / norm, levels[q] / norm});
                    }
                }
            }
            break;
        case LIQUID_MODEM_256QAM:
            m_order = 256;
            {
                float levels[] = {-15.0f, -13.0f, -11.0f, -9.0f, -7.0f, -5.0f, -3.0f, -1.0f,
                                  1.0f, 3.0f, 5.0f, 7.0f, 9.0f, 11.0f, 13.0f, 15.0f};
                float norm = std::sqrt(170.0f);
                for (int i = 0; i < 16; i++) {
                    for (int q = 0; q < 16; q++) {
                        m_constellation.push_back({levels[i] / norm, levels[q] / norm});
                    }
                }
            }
            break;
        default:
            m_order = 4;
            m_constellation = {
                {0.7071f, 0.7071f}, {-0.7071f, 0.7071f},
                {-0.7071f, -0.7071f}, {0.7071f, -0.7071f}
            };
            break;
    }
}

void Modem::modulate(unsigned int symbol_in, std::complex<float>& symbol_out) {
    if (symbol_in < m_order) {
        symbol_out = m_constellation[symbol_in];
    } else {
        symbol_out = {0.0f, 0.0f};
    }
}

void Modem::demodulate(std::complex<float> symbol_in, unsigned int& symbol_out) {
    float min_dist = std::numeric_limits<float>::max();
    symbol_out = 0;
    
    for (unsigned int i = 0; i < m_order; i++) {
        float dr = symbol_in.real() - m_constellation[i].real();
        float di = symbol_in.imag() - m_constellation[i].imag();
        float dist = dr * dr + di * di;
        if (dist < min_dist) {
            min_dist = dist;
            symbol_out = i;
        }
    }
}

} // namespace liquid
