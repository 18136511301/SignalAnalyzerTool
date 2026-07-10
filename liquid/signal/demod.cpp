#include "demod.h"
#include "../filter/firdes.h"
#include <cmath>
#include <cstring>

namespace liquid {

const float Demodulator::PI = 3.14159265358979323846f;

Demodulator::Demodulator(ModulationType type, float symbol_rate, float sample_rate)
    : m_modem(type), m_nco(NCO_COMPLEX), m_agc(0.02f),
      m_matched_filter(nullptr), m_sample_rate(sample_rate),
      m_symbol_rate(symbol_rate), m_samples_per_symbol(sample_rate / symbol_rate),
      m_sample_counter(0), m_centerFreq(0.0f),
      m_pll_freq(0.0f),
      m_pll_alpha(0.01f), m_pll_beta(0.0001f),
      m_pll_locked(false), m_pll_lock_avg(0.0f), m_agc_rssi(0.0f) {
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
    // Matched filter: root raised cosine or simple integrate-and-dump
    unsigned int sps = static_cast<unsigned int>(m_samples_per_symbol);
    unsigned int n = sps * 4;  // 4 symbol lengths
    if (n < 31) n = 31;
    if (n % 2 == 0) n++;

    // Use Kaiser lowpass with cutoff at 0.5 * symbol_rate / sample_rate
    // (half the symbol bandwidth, for matched filtering)
    float fc = 0.5f * symbol_rate / sample_rate;
    auto taps = FirDes::design_kaiser_lowpass(n, fc, 60.0f);
    m_matched_filter = new FirFilter(taps);
}

// ============================================================
//  V2 API: Full demodulation
// ============================================================

int Demodulator::demodulate_v2(const DemodInput* input, DemodOutput* output) {
    if (!input || !output) return -1;
    if (!input->data || input->data_len == 0) return -1;
    if (!output->hard_symbols && !output->soft_symbols) return -1;

    m_sample_rate = input->sample_rate;
    m_symbol_rate = input->symbol_rate;
    m_samples_per_symbol = m_sample_rate / m_symbol_rate;

    // Re-init filter with new parameters
    delete m_matched_filter;
    m_matched_filter = nullptr;
    init_filter(m_symbol_rate, m_sample_rate);

    // Set up modem
    Modem modem(static_cast<ModulationType>(input->modulation_type));
    m_modem = modem;

    // Set up NCO for carrier downconversion
    m_centerFreq = input->carrier_freq;
    float dtheta = 2.0f * PI * m_centerFreq / m_sample_rate;
    m_nco.set_frequency(dtheta);
    m_nco.set_phase(0.0f);

    // Reset PLL
    m_pll_freq = 0.0f;
    m_pll_locked = false;
    m_pll_lock_avg = 0.0f;

    // Reset AGC
    m_agc.reset();
    m_sample_counter = 0;

    // 1) Convert raw data to float I/Q
    std::vector<float> i_data, q_data;
    unsigned int num_samples = 0;

    if (input->bit_width == 16) {
        unsigned int num_int16 = input->data_len / sizeof(int16_t);
        const int16_t* raw = reinterpret_cast<const int16_t*>(input->data);
        if (input->signal_type == LIQUID_SIGNAL_REAL) {
            num_samples = num_int16;
            i_data.resize(num_samples);
            for (unsigned int i = 0; i < num_samples; i++)
                i_data[i] = static_cast<float>(raw[i]) / 32768.0f;
        } else {
            num_samples = num_int16 / 2;
            i_data.resize(num_samples);
            q_data.resize(num_samples);
            for (unsigned int i = 0; i < num_samples; i++) {
                i_data[i] = static_cast<float>(raw[2 * i]) / 32768.0f;
                q_data[i] = static_cast<float>(raw[2 * i + 1]) / 32768.0f;
            }
        }
    } else if (input->bit_width == 32) {
        unsigned int num_float = input->data_len / sizeof(float);
        const float* raw = reinterpret_cast<const float*>(input->data);
        if (input->signal_type == LIQUID_SIGNAL_REAL) {
            num_samples = num_float;
            i_data.resize(num_samples);
            std::memcpy(i_data.data(), raw, num_samples * sizeof(float));
        } else {
            num_samples = num_float / 2;
            i_data.resize(num_samples);
            q_data.resize(num_samples);
            for (unsigned int i = 0; i < num_samples; i++) {
                i_data[i] = raw[2 * i];
                q_data[i] = raw[2 * i + 1];
            }
        }
    } else {
        return -1;  // Unsupported bit width
    }

    if (num_samples < 64) return -1;

    // 2) Build complex IQ buffer
    std::vector<std::complex<float>> iq(num_samples);
    if (input->signal_type == LIQUID_SIGNAL_REAL) {
        for (unsigned int i = 0; i < num_samples; i++)
            iq[i] = std::complex<float>(i_data[i], 0.0f);
    } else {
        for (unsigned int i = 0; i < num_samples; i++)
            iq[i] = std::complex<float>(i_data[i], q_data[i]);
    }

    // 3) Process through demod chain
    std::vector<unsigned int> hard_symbols;
    std::vector<std::complex<float>> soft_symbols;
    process_block(iq.data(), num_samples, hard_symbols, soft_symbols);

    // 4) Copy results to output
    output->hard_count = 0;
    output->soft_count = 0;

    if (output->hard_symbols && output->hard_capacity > 0) {
        unsigned int n = static_cast<unsigned int>(hard_symbols.size());
        if (n > output->hard_capacity) n = output->hard_capacity;
        std::memcpy(output->hard_symbols, hard_symbols.data(), n * sizeof(unsigned int));
        output->hard_count = n;
    }

    if (output->soft_symbols && output->soft_capacity > 0) {
        unsigned int n = static_cast<unsigned int>(soft_symbols.size());
        if (n > output->soft_capacity) n = output->soft_capacity;
        std::memcpy(output->soft_symbols, soft_symbols.data(), n * sizeof(ComplexFloat));
        output->soft_count = n;
    }

    output->locked = m_pll_locked;
    output->freq_offset = m_pll_freq * m_symbol_rate / (2.0f * PI);

    return 0;
}

// ============================================================
//  Internal: process block through demod chain
// ============================================================

void Demodulator::process_block(const std::complex<float>* iq_data, unsigned int n,
                                 std::vector<unsigned int>& hard_out,
                                 std::vector<std::complex<float>>& soft_out) {
    hard_out.clear();
    soft_out.clear();
    hard_out.reserve(n / static_cast<unsigned int>(m_samples_per_symbol) + 1);
    soft_out.reserve(n / static_cast<unsigned int>(m_samples_per_symbol) + 1);

    float pll_err_avg = 0.0f;
    unsigned int pll_err_cnt = 0;

    for (unsigned int i = 0; i < n; i++) {
        std::complex<float> sample = iq_data[i];

        // NCO downconversion
        std::complex<float> nco_out;
        m_nco.execute(nco_out);
        sample *= std::conj(nco_out);

        // AGC
        m_agc.execute(sample);

        // Matched filter
        std::complex<float> filtered;
        m_matched_filter->execute(sample, filtered);

        // Symbol timing: count samples, strobe at symbol rate
        m_sample_counter++;
        if (m_sample_counter >= static_cast<unsigned int>(m_samples_per_symbol)) {
            m_sample_counter = 0;

            // Store soft symbol (I/Q before hard decision)
            soft_out.push_back(filtered);
            m_softSymbols.push_back(filtered);
            if (m_softSymbols.size() > 4096)
                m_softSymbols.erase(m_softSymbols.begin(),
                                    m_softSymbols.begin() + (m_softSymbols.size() - 4096));

            // Costas loop: decision-directed phase tracking
            float pll_err = 0.0f;
            if (m_modem.type() == LIQUID_MODEM_BPSK) {
                // BPSK: error = sign(I) * Q
                pll_err = (filtered.real() >= 0.0f ? 1.0f : -1.0f) * filtered.imag();
            } else if (m_modem.type() == LIQUID_MODEM_QPSK) {
                // QPSK: error = sign(I)*Q - sign(Q)*I
                float sI = (filtered.real() >= 0.0f ? 1.0f : -1.0f);
                float sQ = (filtered.imag() >= 0.0f ? 1.0f : -1.0f);
                pll_err = sI * filtered.imag() - sQ * filtered.real();
            } else {
                // Generic: use decision-directed error
                pll_err = (filtered.real() >= 0.0f ? 1.0f : -1.0f) * filtered.imag();
            }

            // PLL update
            m_pll_freq += m_pll_beta * pll_err;
            float base_dtheta = 2.0f * PI * m_centerFreq / m_sample_rate;
            float pll_correction = m_pll_alpha * pll_err + m_pll_freq;
            m_nco.set_frequency(base_dtheta + pll_correction * m_symbol_rate / m_sample_rate);

            // Lock detector (exponential averaging of |error|)
            m_pll_lock_avg = 0.99f * m_pll_lock_avg + 0.01f * std::abs(pll_err);
            m_pll_locked = (m_pll_lock_avg < 0.3f);

            // Hard decision
            unsigned int symbol;
            m_modem.demodulate(filtered, symbol);
            hard_out.push_back(symbol);
        }
    }
}

// ============================================================
//  Legacy API
// ============================================================

int Demodulator::execute(const std::complex<float>* iq_data, unsigned int n, 
                          unsigned int* symbols, unsigned int& num_symbols) {
    num_symbols = 0;
    
    for (unsigned int i = 0; i < n; i++) {
        std::complex<float> sample = iq_data[i];
        
        std::complex<float> nco_out;
        m_nco.execute(nco_out);
        sample *= std::conj(nco_out);
        
        m_agc.execute(sample);
        
        std::complex<float> filtered;
        m_matched_filter->execute(sample, filtered);
        
        m_sample_counter++;
        if (m_sample_counter >= static_cast<unsigned int>(m_samples_per_symbol)) {
            m_sample_counter = 0;
            
            m_softSymbols.push_back(filtered);
            if (m_softSymbols.size() > 4096)
                m_softSymbols.erase(m_softSymbols.begin(),
                                    m_softSymbols.begin() + (m_softSymbols.size() - 4096));
            
            float pll_err = (filtered.real() >= 0.0f ? 1.0f : -1.0f) * filtered.imag();
            m_pll_freq += m_pll_beta * pll_err;
            
            float base_dtheta = 2.0f * PI * m_centerFreq / m_sample_rate;
            float pll_correction = m_pll_alpha * pll_err + m_pll_freq;
            m_nco.set_frequency(base_dtheta + pll_correction * m_symbol_rate / m_sample_rate);
            
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
