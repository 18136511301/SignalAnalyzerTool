#include "demod.h"
#include "../filter/firdes.h"
#include <cmath>

namespace liquid {

const float Demodulator::PI = 3.14159265358979323846f;

Demodulator::Demodulator(ModulationType type, float symbol_rate, float sample_rate)
    : m_modem(type), m_nco(NCO_COMPLEX), m_agc(0.01f),
      m_matched_filter(nullptr), m_sample_rate(sample_rate),
      m_symbol_rate(symbol_rate), m_samples_per_symbol(sample_rate / symbol_rate),
      m_sample_counter(0), m_centerFreq(0.0f),
      m_pll_phase(0.0f), m_pll_freq(0.0f),
      m_pll_alpha(0.008f), m_pll_beta(0.00004f) {
    init_filter(symbol_rate, sample_rate);
}

Demodulator::~Demodulator() {
    delete m_matched_filter;
}

void Demodulator::set_center_freq(float freq_hz) {
    m_centerFreq = freq_hz;
    float dtheta = 2.0f * PI * freq_hz / m_sample_rate;
    m_nco.set_frequency(dtheta);
}

void Demodulator::init_filter(float symbol_rate, float sample_rate) {
    unsigned int n = static_cast<unsigned int>(m_samples_per_symbol) * 4;
    if (n < 31) n = 31;
    if (n % 2 == 0) n++;
    
    float fc = symbol_rate / sample_rate;
    auto taps = FirDes::design_kaiser_lowpass(n, fc, 60.0f);
    
    std::vector<std::complex<float>> complex_taps(taps.size());
    for (unsigned int i = 0; i < taps.size(); i++) {
        complex_taps[i] = std::complex<float>(taps[i], 0.0f);
    }
    m_matched_filter_taps = complex_taps;
    
    std::vector<float> real_taps(taps.begin(), taps.end());
    m_matched_filter = new FirFilter(real_taps);
}

int Demodulator::execute(const std::complex<float>* iq_data, unsigned int n, 
                         unsigned int* symbols, unsigned int& num_symbols) {
    num_symbols = 0;
    
    for (unsigned int i = 0; i < n; i++) {
        std::complex<float> sample = iq_data[i];
        
        // NCO downconversion: shift carrier to baseband
        std::complex<float> nco_out;
        m_nco.execute(nco_out);
        sample *= std::conj(nco_out);
        
        m_agc.execute(sample);
        
        std::complex<float> filtered;
        m_matched_filter->execute(sample, filtered);
        
        m_sample_counter++;
        if (m_sample_counter >= static_cast<unsigned int>(m_samples_per_symbol)) {
            m_sample_counter = 0;
            
            // Store soft symbol (matched filter output before decision)
            m_softSymbols.push_back(filtered);
            if (m_softSymbols.size() > 4096)
                m_softSymbols.erase(m_softSymbols.begin(),
                                    m_softSymbols.begin() + (m_softSymbols.size() - 4096));
            
            // Costas loop: decision-directed phase tracking
            // error = sign(I) * Q  (avoids 90-degree false lock)
            float pll_err = (filtered.real() >= 0.0f ? 1.0f : -1.0f) * filtered.imag();
            m_pll_freq += m_pll_beta * pll_err;
            
            float base_dtheta = 2.0f * PI * m_centerFreq / m_sample_rate;
            float pll_correction = m_pll_alpha * pll_err + m_pll_freq;
            m_nco.set_frequency(base_dtheta + pll_correction / m_samples_per_symbol);
            
            unsigned int symbol;
            m_modem.demodulate(filtered, symbol);
            
            if (symbols && num_symbols < n) {
                symbols[num_symbols] = symbol;
            }
            num_symbols++;
        }
    }
    
    return LIQUID_OK;
}

int Demodulator::execute_single(std::complex<float> sample, unsigned int& symbol) {
    // NCO downconversion
    std::complex<float> nco_out;
    m_nco.execute(nco_out);
    sample *= std::conj(nco_out);
    
    m_agc.execute(sample);
    
    std::complex<float> filtered;
    m_matched_filter->execute(sample, filtered);
    
    m_sample_counter++;
    if (m_sample_counter >= static_cast<unsigned int>(m_samples_per_symbol)) {
        m_sample_counter = 0;
        m_modem.demodulate(filtered, symbol);
        return LIQUID_OK;
    }
    
    return -1;
}

void Demodulator::get_soft_symbols(std::complex<float>* buffer, unsigned int& n) {
    unsigned int count = static_cast<unsigned int>(m_softSymbols.size());
    if (count > n) count = n;
    for (unsigned int i = 0; i < count; i++)
        buffer[i] = m_softSymbols[i];
    n = count;
}

} // namespace liquid
