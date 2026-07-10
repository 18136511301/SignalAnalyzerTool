#pragma once
#include "../core/types.h"
#include "../modem/modem.h"
#include "../nco/nco.h"
#include "../filter/firfilt.h"
#include "../agc/agc.h"
#include <complex>
#include <vector>

namespace liquid {

class Demodulator {
public:
    Demodulator(ModulationType type, float symbol_rate, float sample_rate);
    ~Demodulator();

    // Legacy API
    int execute(const std::complex<float>* iq_data, unsigned int n, 
                unsigned int* symbols, unsigned int& num_symbols);
    int execute_single(std::complex<float> sample, unsigned int& symbol);
    void set_center_freq(float freq_hz);
    void get_soft_symbols(std::complex<float>* buffer, unsigned int& n);

    // V2 API: full demodulation with hard + soft output
    int demodulate_v2(const DemodInput* input, DemodOutput* output);

private:
    // Internal: process a block of complex samples through the demod chain
    void process_block(const std::complex<float>* iq_data, unsigned int n,
                       std::vector<unsigned int>& hard_out,
                       std::vector<std::complex<float>>& soft_out);

    void init_filter(float symbol_rate, float sample_rate);
    
    Modem m_modem;
    Nco m_nco;
    Agc m_agc;
    FirFilter* m_matched_filter;
    float m_sample_rate;
    float m_symbol_rate;
    float m_samples_per_symbol;
    unsigned int m_sample_counter;
    static const float PI;
    std::vector<std::complex<float>> m_softSymbols;
    float m_centerFreq;
    
    // Costas loop for carrier tracking
    float m_pll_freq;       // NCO frequency correction (rad/symbol)
    float m_pll_alpha;      // Proportional gain
    float m_pll_beta;       // Integrator gain
    bool  m_pll_locked;     // Lock detector
    float m_pll_lock_avg;   // Lock detector averaging
    float m_agc_rssi;       // RSSI for SNR estimation
};

} // namespace liquid
