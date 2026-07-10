#pragma once
#include "../core/types.h"
#include "../modem/modem.h"
#include "../nco/nco.h"
#include "../filter/firfilt.h"
#include "../agc/agc.h"
#include <complex>

namespace liquid {

class Demodulator {
public:
    Demodulator(ModulationType type, float symbol_rate, float sample_rate);
    ~Demodulator();

    int execute(const std::complex<float>* iq_data, unsigned int n, 
                unsigned int* symbols, unsigned int& num_symbols);
    
    int execute_single(std::complex<float> sample, unsigned int& symbol);
    
    void set_center_freq(float freq_hz);
    
    void get_soft_symbols(std::complex<float>* buffer, unsigned int& n);

private:
    void init_filter(float symbol_rate, float sample_rate);
    
    Modem m_modem;
    Nco m_nco;
    Agc m_agc;
    std::vector<std::complex<float>> m_matched_filter_taps;
    FirFilter* m_matched_filter;
    float m_sample_rate;
    float m_symbol_rate;
    float m_samples_per_symbol;
    unsigned int m_sample_counter;
    static const float PI;
    std::vector<std::complex<float>> m_softSymbols;
    float m_centerFreq;
    
    // Costas loop for carrier tracking
    float m_pll_phase;
    float m_pll_freq;
    float m_pll_alpha;
    float m_pll_beta;
};

} // namespace liquid
