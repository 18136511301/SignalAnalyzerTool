#pragma once
#include "../core/types.h"
#include <complex>
#include <vector>

namespace liquid {

class SignalIdentifier {
public:
    SignalIdentifier(float sample_rate);
    ~SignalIdentifier();

    // Legacy API
    SignalInfo identify(const std::complex<float>* iq_data, unsigned int n);
    SignalInfo identify(const float* iq_data, unsigned int n, bool is_complex = false);

    // V2 API: accepts raw binary data
    int identify_v2(const IdentifyParams* params, IdentifyResult* result);

private:
    // Raw data conversion
    static bool convert_to_float(const void* data, unsigned int data_len,
                                 int bit_width, int signal_type,
                                 std::vector<float>& out_i, std::vector<float>& out_q,
                                 unsigned int& num_samples);

    void compute_psd(const std::complex<float>* iq_data, unsigned int n,
                     std::vector<float>& psd, std::vector<float>& freq_axis);
    void compute_psd_real(const float* data, unsigned int n,
                         std::vector<float>& psd, std::vector<float>& freq_axis);
    float estimate_center_freq(const std::vector<float>& psd, const std::vector<float>& freq_axis);
    float estimate_bandwidth_3db(const std::vector<float>& psd, const std::vector<float>& freq_axis);
    float estimate_symbol_rate(const std::complex<float>* iq_data, unsigned int n);
    float estimate_symbol_rate_real(const float* data, unsigned int n);
    int classify_modulation(const std::complex<float>* iq_data, unsigned int n);
    float estimate_snr(const std::complex<float>* iq_data, unsigned int n);
    int get_mod_order(int mod_type);
    int get_bits_per_symbol(int mod_type);
    int get_confidence(const std::complex<float>* iq_data, unsigned int n, int mod_type);

    // Frequency refinement using squared-signal spectrum
    float refine_frequency(const std::complex<float>* iq_data, unsigned int n,
                           float coarse_fc);
    // Lowpass filter
    void lowpass_filter(std::vector<std::complex<float>>& data, float cutoff_hz);
    void lowpass_filter_real(std::vector<float>& data, float cutoff_hz);

    float m_sample_rate;
    static const float PI;
};

} // namespace liquid
