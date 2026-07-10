#pragma once
#include "../core/types.h"
#include <complex>
#include <vector>

namespace liquid {

class SignalIdentifier {
public:
    SignalIdentifier(float sample_rate);
    ~SignalIdentifier();

    SignalInfo identify(const std::complex<float>* iq_data, unsigned int n);
    SignalInfo identify(const float* iq_data, unsigned int n, bool is_complex = false);

private:
    void compute_psd(const std::complex<float>* iq_data, unsigned int n,
                     std::vector<float>& psd, std::vector<float>& freq_axis);
    float estimate_center_freq(const std::vector<float>& psd, const std::vector<float>& freq_axis);
    float estimate_bandwidth(const std::vector<float>& psd, const std::vector<float>& freq_axis);
    float estimate_symbol_rate(const std::complex<float>* iq_data, unsigned int n);
    int classify_modulation(const std::complex<float>* iq_data, unsigned int n);
    float estimate_snr(const std::complex<float>* iq_data, unsigned int n);
    int get_mod_order(int mod_type);

    // Frequency refinement using squared-signal spectrum
    float refine_frequency(const std::complex<float>* iq_data, unsigned int n,
                           float coarse_fc);
    // Simple FIR lowpass filter (in-place on complex vector)
    void lowpass_filter(std::vector<std::complex<float>>& data, float cutoff_hz);

    float m_sample_rate;
    static const float PI;
};

} // namespace liquid
