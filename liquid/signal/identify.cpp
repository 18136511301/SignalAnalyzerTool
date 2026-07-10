#include "identify.h"
#include "../fft/fft.h"
#include "../filter/firfilt.h"
#include "../filter/firdes.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <cstring>

namespace liquid {

const float SignalIdentifier::PI = 3.14159265358979323846f;

SignalIdentifier::SignalIdentifier(float sample_rate) : m_sample_rate(sample_rate) {
}

SignalIdentifier::~SignalIdentifier() {
}

// ============================================================
//  V2 API: Raw binary data → IdentifyResult
// ============================================================

int SignalIdentifier::identify_v2(const IdentifyParams* params, IdentifyResult* result) {
    if (!params || !result || !params->data || params->data_len == 0)
        return -1;
    if (params->sample_rate <= 0)
        return -1;

    m_sample_rate = params->sample_rate;
    std::memset(result, 0, sizeof(IdentifyResult));

    // 1) Convert raw binary data to float I/Q
    std::vector<float> i_data, q_data;
    unsigned int num_samples = 0;
    if (!convert_to_float(params->data, params->data_len, params->bit_width,
                          params->signal_type, i_data, q_data, num_samples)) {
        return -1;
    }
    if (num_samples < 64) return -1;

    // 2) Build complex baseband
    std::vector<std::complex<float>> iq(num_samples);
    if (params->signal_type == LIQUID_SIGNAL_REAL) {
        for (unsigned int i = 0; i < num_samples; i++)
            iq[i] = std::complex<float>(i_data[i], 0.0f);
    } else {
        for (unsigned int i = 0; i < num_samples; i++)
            iq[i] = std::complex<float>(i_data[i], q_data[i]);
    }

    // 3) Estimate center frequency
    if (params->signal_type == LIQUID_SIGNAL_REAL) {
        // Real signal: use squared-signal method to find carrier at 2*fc
        unsigned int fft_size = 4096;
        unsigned int temp = 1;
        while (temp < fft_size && temp < num_samples) temp *= 2;
        fft_size = temp;
        if (fft_size > num_samples) fft_size = num_samples;
        if (fft_size < 64) fft_size = 64;

        FFT fft(fft_size, 1);
        std::vector<std::complex<float>> buf(fft_size);
        for (unsigned int i = 0; i < fft_size; i++) {
            float w = 0.5f * (1.0f - std::cos(2.0f * PI * i / (fft_size - 1)));
            buf[i] = std::complex<float>(i_data[i] * i_data[i] * w, 0.0f);
        }
        fft.execute(buf.data(), buf.data());

        // Search for 2*fc tone in positive frequencies [fs/6, fs/2)
        // fs/6 avoids DC; fs/2 avoids Nyquist edge
        unsigned int lo = fft_size / 6;
        unsigned int hi = fft_size / 2 - 1;
        float max_mag = 0.0f;
        unsigned int max_bin = lo;
        for (unsigned int i = lo; i <= hi; i++) {
            float mag = std::abs(buf[i]);
            if (mag > max_mag) { max_mag = mag; max_bin = i; }
        }

        float freq_res = m_sample_rate / fft_size;
        float peak_freq = max_bin * freq_res;

        // Parabolic interpolation for sub-bin accuracy
        if (max_bin > lo && max_bin < hi) {
            float a = std::abs(buf[max_bin - 1]);
            float b = std::abs(buf[max_bin]);
            float c = std::abs(buf[max_bin + 1]);
            float denom = a - 2.0f * b + c;
            if (std::abs(denom) > 0.01f) {
                float p = 0.5f * (a - c) / denom;
                if (std::abs(p) < 1.0f) peak_freq += p * freq_res;
            }
        }
        result->center_freq = peak_freq * 0.5f;

        // Downconvert to baseband
        float mix_freq = -result->center_freq;
        std::vector<std::complex<float>> baseband(num_samples);
        for (unsigned int i = 0; i < num_samples; i++) {
            float phase = 2.0f * PI * mix_freq * i / m_sample_rate;
            baseband[i] = std::complex<float>(i_data[i], 0.0f) *
                          std::complex<float>(std::cos(phase), std::sin(phase));
        }
        lowpass_filter(baseband, m_sample_rate * 0.25f);

        // Refine carrier frequency on baseband
        float residual = refine_frequency(baseband.data(), num_samples, result->center_freq);
        if (std::abs(residual) > 0.5f) {
            result->center_freq += residual;
            // Re-downconvert with refined frequency
            float new_mix = -result->center_freq;
            for (unsigned int i = 0; i < num_samples; i++) {
                float phase = 2.0f * PI * new_mix * i / m_sample_rate;
                baseband[i] = std::complex<float>(i_data[i], 0.0f) *
                              std::complex<float>(std::cos(phase), std::sin(phase));
            }
            lowpass_filter(baseband, m_sample_rate * 0.25f);
        }

        // 4) Bandwidth (3dB)
        {
            std::vector<float> bb_psd, bb_freq;
            compute_psd(baseband.data(), num_samples, bb_psd, bb_freq);
            result->bandwidth = estimate_bandwidth_3db(bb_psd, bb_freq);
        }

        // 5) Symbol rate (delay-multiply method for real signals)
        result->symbol_rate = estimate_symbol_rate_real(i_data.data(), num_samples);

        // 6) Modulation classification
        unsigned int classify_n = (num_samples < 4000) ? num_samples : 4000;
        result->modulation_type = classify_modulation(baseband.data(), classify_n);
        result->modulation_order = get_mod_order(result->modulation_type);
        result->bits_per_symbol = get_bits_per_symbol(result->modulation_type);
        result->confidence = get_confidence(baseband.data(), classify_n, result->modulation_type);

        // 7) SNR
        result->snr = estimate_snr(baseband.data(), num_samples);
    } else {
        // Complex IQ path
        std::vector<float> psd, freq_axis;
        compute_psd(iq.data(), num_samples, psd, freq_axis);
        result->center_freq = estimate_center_freq(psd, freq_axis);

        // Downconvert to baseband
        float mix_freq = -result->center_freq;
        std::vector<std::complex<float>> baseband(num_samples);
        for (unsigned int i = 0; i < num_samples; i++) {
            float phase = 2.0f * PI * mix_freq * i / m_sample_rate;
            std::complex<float> nco_val(std::cos(phase), std::sin(phase));
            baseband[i] = iq[i] * nco_val;
        }
        lowpass_filter(baseband, m_sample_rate * 0.25f);

        // Refine frequency
        float residual = refine_frequency(baseband.data(), num_samples, result->center_freq);
        if (std::abs(residual) > 0.5f) {
            result->center_freq += residual;
            float new_mix = -result->center_freq;
            for (unsigned int i = 0; i < num_samples; i++) {
                float phase = 2.0f * PI * new_mix * i / m_sample_rate;
                std::complex<float> nco_val(std::cos(phase), std::sin(phase));
                baseband[i] = iq[i] * nco_val;
            }
            lowpass_filter(baseband, m_sample_rate * 0.25f);
        }

        // Bandwidth
        {
            std::vector<float> bb_psd, bb_freq;
            compute_psd(baseband.data(), num_samples, bb_psd, bb_freq);
            result->bandwidth = estimate_bandwidth_3db(bb_psd, bb_freq);
        }

        // Symbol rate
        result->symbol_rate = estimate_symbol_rate(baseband.data(), num_samples);

        // Modulation
        unsigned int classify_n = (num_samples < 4000) ? num_samples : 4000;
        result->modulation_type = classify_modulation(baseband.data(), classify_n);
        result->modulation_order = get_mod_order(result->modulation_type);
        result->bits_per_symbol = get_bits_per_symbol(result->modulation_type);
        result->confidence = get_confidence(baseband.data(), classify_n, result->modulation_type);

        // SNR
        result->snr = estimate_snr(iq.data(), num_samples);
    }

    return 0;
}

// ============================================================
//  Raw binary data conversion
// ============================================================

bool SignalIdentifier::convert_to_float(const void* data, unsigned int data_len,
                                         int bit_width, int signal_type,
                                         std::vector<float>& out_i, std::vector<float>& out_q,
                                         unsigned int& num_samples) {
    if (!data || data_len == 0) return false;

    if (bit_width == 16) {
        // 16-bit signed integer (int16_t)
        unsigned int num_int16 = data_len / sizeof(int16_t);
        const int16_t* raw = reinterpret_cast<const int16_t*>(data);

        if (signal_type == LIQUID_SIGNAL_REAL) {
            num_samples = num_int16;
            out_i.resize(num_samples);
            out_q.clear();
            for (unsigned int i = 0; i < num_samples; i++)
                out_i[i] = static_cast<float>(raw[i]) / 32768.0f;
        } else {
            // Complex: I/Q interleaved
            num_samples = num_int16 / 2;
            out_i.resize(num_samples);
            out_q.resize(num_samples);
            for (unsigned int i = 0; i < num_samples; i++) {
                out_i[i] = static_cast<float>(raw[2 * i]) / 32768.0f;
                out_q[i] = static_cast<float>(raw[2 * i + 1]) / 32768.0f;
            }
        }
        return true;
    } else if (bit_width == 32) {
        // 32-bit float
        unsigned int num_float = data_len / sizeof(float);
        const float* raw = reinterpret_cast<const float*>(data);

        if (signal_type == LIQUID_SIGNAL_REAL) {
            num_samples = num_float;
            out_i.resize(num_samples);
            out_q.clear();
            std::memcpy(out_i.data(), raw, num_samples * sizeof(float));
        } else {
            num_samples = num_float / 2;
            out_i.resize(num_samples);
            out_q.resize(num_samples);
            for (unsigned int i = 0; i < num_samples; i++) {
                out_i[i] = raw[2 * i];
                out_q[i] = raw[2 * i + 1];
            }
        }
        return true;
    }

    return false;  // Unsupported bit width
}

// ============================================================
//  Legacy API (backward compatible)
// ============================================================

SignalInfo SignalIdentifier::identify(const std::complex<float>* iq_data, unsigned int n) {
    SignalInfo info;
    if (n < 64) return info;

    std::vector<std::complex<float>> data(iq_data, iq_data + n);

    // 1. Compute PSD and find center frequency
    std::vector<float> psd;
    std::vector<float> freq_axis;
    compute_psd(data.data(), n, psd, freq_axis);
    info.center_freq = estimate_center_freq(psd, freq_axis);

    // 2. Downconvert to baseband
    float mix_freq = -info.center_freq;
    std::vector<std::complex<float>> baseband(n);
    for (unsigned int i = 0; i < n; i++) {
        float phase = 2.0f * PI * mix_freq * i / m_sample_rate;
        std::complex<float> nco_val(std::cos(phase), std::sin(phase));
        baseband[i] = data[i] * nco_val;
    }

    lowpass_filter(baseband, m_sample_rate * 0.25f);

    // Refine center frequency
    float residual = refine_frequency(baseband.data(), n, info.center_freq);
    info.center_freq += residual;

    if (std::abs(residual) > 0.5f) {
        mix_freq = -info.center_freq;
        for (unsigned int i = 0; i < n; i++) {
            float phase = 2.0f * PI * mix_freq * i / m_sample_rate;
            std::complex<float> nco_val(std::cos(phase), std::sin(phase));
            baseband[i] = data[i] * nco_val;
        }
        lowpass_filter(baseband, m_sample_rate * 0.25f);
    }

    // 3. Bandwidth
    std::vector<float> bb_psd, bb_freq;
    compute_psd(baseband.data(), n, bb_psd, bb_freq);
    info.bandwidth = estimate_bandwidth_3db(bb_psd, bb_freq);

    // 4. Symbol rate
    info.symbol_rate = estimate_symbol_rate(baseband.data(), n);

    // 5. Modulation
    info.modulation_type = classify_modulation(baseband.data(), n);
    info.modulation_order = get_mod_order(info.modulation_type);

    // 6. SNR
    info.snr = estimate_snr(data.data(), n);

    return info;
}

SignalInfo SignalIdentifier::identify(const float* iq_data, unsigned int n, bool is_complex) {
    if (is_complex) {
        unsigned int num_samples = n / 2;
        std::vector<std::complex<float>> complex_data(num_samples);
        for (unsigned int i = 0; i < num_samples; i++) {
            complex_data[i] = std::complex<float>(iq_data[2 * i], iq_data[2 * i + 1]);
        }
        return identify(complex_data.data(), num_samples);
    }

    // Real signal path
    SignalInfo info;
    if (n < 64) return info;

    // 1) Find carrier via squared-signal method
    unsigned int fft_size = 4096;
    unsigned int temp = 1;
    while (temp < fft_size && temp < n) temp *= 2;
    fft_size = temp;
    if (fft_size > n) fft_size = n;
    if (fft_size < 64) fft_size = 64;

    {
        FFT fft(fft_size, 1);
        std::vector<std::complex<float>> buf(fft_size);
        for (unsigned int i = 0; i < fft_size; i++) {
            float w = 0.5f * (1.0f - std::cos(2.0f * PI * i / (fft_size - 1)));
            buf[i] = std::complex<float>(iq_data[i] * iq_data[i] * w, 0.0f);
        }
        fft.execute(buf.data(), buf.data());

        unsigned int lo = fft_size / 6;
        unsigned int hi = fft_size / 2 - 1;
        float max_mag = 0.0f;
        unsigned int max_bin = lo;
        for (unsigned int i = lo; i <= hi; i++) {
            float mag = std::abs(buf[i]);
            if (mag > max_mag) { max_mag = mag; max_bin = i; }
        }
        if (max_bin > lo) {
            float freq_res = m_sample_rate / fft_size;
            float peak_freq = max_bin * freq_res;
            if (max_bin > lo && max_bin < hi) {
                float a = std::abs(buf[max_bin - 1]);
                float b = std::abs(buf[max_bin]);
                float c = std::abs(buf[max_bin + 1]);
                float denom = a - 2.0f * b + c;
                if (std::abs(denom) > 0.01f) {
                    float p = 0.5f * (a - c) / denom;
                    if (std::abs(p) < 1.0f) peak_freq += p * freq_res;
                }
            }
            info.center_freq = peak_freq * 0.5f;
        }
    }

    // 2) Downconvert to baseband
    float mix_freq = -info.center_freq;
    std::vector<std::complex<float>> baseband(n);
    for (unsigned int i = 0; i < n; i++) {
        float phase = 2.0f * PI * mix_freq * i / m_sample_rate;
        baseband[i] = std::complex<float>(iq_data[i], 0.0f) *
                       std::complex<float>(std::cos(phase), std::sin(phase));
    }

    lowpass_filter(baseband, m_sample_rate * 0.25f);

    // Refine frequency
    {
        FFT fft_sq(2048, 1);
        std::vector<std::complex<float>> sq_buf(2048);
        unsigned int sq_n = (n < 2048) ? n : 2048;
        for (unsigned int i = 0; i < sq_n; i++) {
            float w = 0.5f * (1.0f - std::cos(2.0f * PI * i / (sq_n - 1)));
            sq_buf[i] = baseband[i] * baseband[i] * w;
        }
        std::complex<float> sq_mean(0.0f, 0.0f);
        for (unsigned int i = 0; i < sq_n; i++) sq_mean += sq_buf[i];
        sq_mean /= static_cast<float>(sq_n);
        for (unsigned int i = 0; i < sq_n; i++) sq_buf[i] -= sq_mean;

        fft_sq.execute(sq_buf.data(), sq_buf.data());

        float max_mag = 0.0f;
        unsigned int max_bin = 0;
        float freq_res = m_sample_rate / 2048.0f;
        unsigned int search_limit = 2048 / 32;
        for (unsigned int i = 1; i < search_limit; i++) {
            float mag = std::abs(sq_buf[i]);
            if (mag > max_mag) { max_mag = mag; max_bin = i; }
        }
        if (max_bin > 0) {
            float residual = max_bin * freq_res * 0.5f;
            if (std::abs(residual) > 0.5f) {
                info.center_freq += residual;
                float new_mix = -info.center_freq;
                for (unsigned int i = 0; i < n; i++) {
                    float phase = 2.0f * PI * new_mix * i / m_sample_rate;
                    baseband[i] = std::complex<float>(iq_data[i], 0.0f) *
                                   std::complex<float>(std::cos(phase), std::sin(phase));
                }
                lowpass_filter(baseband, m_sample_rate * 0.25f);
            }
        }
    }

    // 3) Bandwidth
    {
        std::vector<float> bb_psd, bb_freq;
        compute_psd(baseband.data(), n, bb_psd, bb_freq);
        info.bandwidth = estimate_bandwidth_3db(bb_psd, bb_freq);
    }

    // 4) Symbol rate (delay-multiply)
    info.symbol_rate = estimate_symbol_rate_real(iq_data, n);

    // 5) Modulation
    unsigned int classify_n = (n < 2000) ? n : 2000;
    info.modulation_type = classify_modulation(baseband.data(), classify_n);
    info.modulation_order = get_mod_order(info.modulation_type);

    // 6) SNR
    info.snr = estimate_snr(baseband.data(), n);

    return info;
}

// ============================================================
//  PSD computation (complex input, fftshift for [-fs/2, +fs/2])
// ============================================================

void SignalIdentifier::compute_psd(const std::complex<float>* iq_data, unsigned int n,
                                   std::vector<float>& psd, std::vector<float>& freq_axis) {
    unsigned int fft_size = 1024;
    unsigned int temp = 1;
    while (temp < fft_size && temp < n) temp *= 2;
    fft_size = temp;
    if (fft_size > n) fft_size = n;
    if (fft_size < 64) fft_size = 64;

    unsigned int num_avg = n / fft_size;
    if (num_avg < 1) num_avg = 1;

    psd.assign(fft_size, 0.0f);
    FFT fft(fft_size, 1);
    std::vector<std::complex<float>> buf(fft_size);

    std::vector<float> window(fft_size);
    for (unsigned int i = 0; i < fft_size; i++) {
        window[i] = 0.5f * (1.0f - std::cos(2.0f * PI * i / (fft_size - 1)));
    }

    for (unsigned int avg = 0; avg < num_avg; avg++) {
        unsigned int offset = avg * fft_size;
        for (unsigned int i = 0; i < fft_size; i++) {
            buf[i] = iq_data[offset + i] * window[i];
        }
        fft.execute(buf.data(), buf.data());
        for (unsigned int i = 0; i < fft_size; i++) {
            psd[i] += buf[i].real() * buf[i].real() + buf[i].imag() * buf[i].imag();
        }
    }

    for (unsigned int i = 0; i < fft_size; i++) {
        psd[i] = 10.0f * std::log10(psd[i] / num_avg + 1e-20f);
    }

    std::rotate(psd.begin(), psd.begin() + fft_size / 2, psd.end());
    freq_axis.resize(fft_size);
    float freq_res = m_sample_rate / fft_size;
    for (unsigned int i = 0; i < fft_size; i++) {
        freq_axis[i] = (static_cast<float>(i) - fft_size / 2.0f) * freq_res;
    }
}

// ============================================================
//  PSD for real signals (positive frequencies only: 0 ~ fs/2)
// ============================================================

void SignalIdentifier::compute_psd_real(const float* data, unsigned int n,
                                         std::vector<float>& psd, std::vector<float>& freq_axis) {
    unsigned int fft_size = 1024;
    unsigned int temp = 1;
    while (temp < fft_size && temp < n) temp *= 2;
    fft_size = temp;
    if (fft_size > n) fft_size = n;
    if (fft_size < 64) fft_size = 64;

    unsigned int num_avg = n / fft_size;
    if (num_avg < 1) num_avg = 1;

    std::vector<float> raw_psd(fft_size, 0.0f);
    FFT fft(fft_size, 1);
    std::vector<std::complex<float>> buf(fft_size);

    std::vector<float> window(fft_size);
    for (unsigned int i = 0; i < fft_size; i++) {
        window[i] = 0.5f * (1.0f - std::cos(2.0f * PI * i / (fft_size - 1)));
    }

    for (unsigned int avg = 0; avg < num_avg; avg++) {
        unsigned int offset = avg * fft_size;
        for (unsigned int i = 0; i < fft_size; i++) {
            buf[i] = std::complex<float>(data[offset + i] * window[i], 0.0f);
        }
        fft.execute(buf.data(), buf.data());
        for (unsigned int i = 0; i < fft_size; i++) {
            raw_psd[i] += buf[i].real() * buf[i].real() + buf[i].imag() * buf[i].imag();
        }
    }

    // Take positive frequencies only: bins [0, fft_size/2]
    unsigned int half = fft_size / 2 + 1;
    psd.resize(half);
    freq_axis.resize(half);
    float freq_res = m_sample_rate / fft_size;
    for (unsigned int i = 0; i < half; i++) {
        psd[i] = 10.0f * std::log10(raw_psd[i] / num_avg + 1e-20f);
        freq_axis[i] = i * freq_res;
    }
}

// ============================================================
//  Center frequency estimation
// ============================================================

float SignalIdentifier::estimate_center_freq(const std::vector<float>& psd,
                                              const std::vector<float>& freq_axis) {
    unsigned int center = psd.size() / 2;
    float max_val = -1e10f;
    unsigned int max_idx = center;

    for (unsigned int i = center + 1; i < psd.size(); i++) {
        if (psd[i] > max_val) { max_val = psd[i]; max_idx = i; }
    }

    float fc = freq_axis[max_idx];
    if (max_idx > 0 && max_idx < psd.size() - 1) {
        float alpha = psd[max_idx - 1];
        float beta = psd[max_idx];
        float gamma = psd[max_idx + 1];
        float denom = alpha - 2.0f * beta + gamma;
        if (std::abs(denom) > 0.01f) {
            float p = 0.5f * (alpha - gamma) / denom;
            if (std::abs(p) < 1.0f) {
                fc += p * (freq_axis[1] - freq_axis[0]);
            }
        }
    }
    return fc;
}

// ============================================================
//  3dB Bandwidth estimation
// ============================================================

float SignalIdentifier::estimate_bandwidth_3db(const std::vector<float>& psd,
                                                const std::vector<float>& freq_axis) {
    // Find peak
    float max_val = -1e10f;
    unsigned int max_idx = 0;
    for (unsigned int i = 0; i < psd.size(); i++) {
        if (psd[i] > max_val) { max_val = psd[i]; max_idx = i; }
    }

    // 3dB threshold
    float threshold = max_val - 3.0f;

    // Find left crossing
    int left_idx = static_cast<int>(max_idx);
    while (left_idx > 0 && psd[left_idx - 1] >= threshold) left_idx--;

    // Find right crossing
    int right_idx = static_cast<int>(max_idx);
    while (right_idx < static_cast<int>(psd.size()) - 1 && psd[right_idx + 1] >= threshold) right_idx++;

    // Interpolate crossings for better accuracy
    float left_freq = freq_axis[left_idx];
    if (left_idx > 0 && psd[left_idx] < threshold) {
        // Interpolate between left_idx and left_idx+1
        float a = psd[left_idx];
        float b = psd[left_idx + 1];
        if (b > a + 1e-20f) {
            float t = (threshold - a) / (b - a);
            left_freq = freq_axis[left_idx] + t * (freq_axis[left_idx + 1] - freq_axis[left_idx]);
        }
    }

    float right_freq = freq_axis[right_idx];
    if (right_idx < static_cast<int>(psd.size()) - 1 && psd[right_idx] < threshold) {
        float a = psd[right_idx - 1];
        float b = psd[right_idx];
        if (a > b + 1e-20f) {
            float t = (threshold - b) / (a - b);
            right_freq = freq_axis[right_idx] - t * (freq_axis[right_idx] - freq_axis[right_idx - 1]);
        }
    }

    float bw = right_freq - left_freq;
    return (bw > 0) ? bw : 0.0f;
}

// ============================================================
//  Frequency refinement via squared-signal spectrum
// ============================================================

float SignalIdentifier::refine_frequency(const std::complex<float>* iq_data, unsigned int n,
                                          float coarse_fc) {
    unsigned int fft_size = 2048;
    unsigned int temp = 1;
    while (temp < fft_size && temp < n) temp *= 2;
    fft_size = temp;
    if (fft_size > n) fft_size = n;
    if (fft_size < 64) fft_size = 64;

    FFT fft(fft_size, 1);
    std::vector<std::complex<float>> buf(fft_size);
    float freq_res = m_sample_rate / fft_size;

    std::complex<float> mean(0.0f, 0.0f);
    for (unsigned int i = 0; i < fft_size; i++) {
        float w = 0.5f * (1.0f - std::cos(2.0f * PI * i / (fft_size - 1)));
        buf[i] = iq_data[i] * iq_data[i] * w;
        mean += buf[i];
    }
    mean /= static_cast<float>(fft_size);
    for (unsigned int i = 0; i < fft_size; i++) {
        buf[i] -= mean;
    }
    fft.execute(buf.data(), buf.data());

    unsigned int search_limit = fft_size / 32;
    if (search_limit < 4) search_limit = 4;
    float max_mag = 0.0f;
    unsigned int max_bin = 0;
    for (unsigned int i = 1; i < search_limit; i++) {
        float mag = std::abs(buf[i]);
        if (mag > max_mag) { max_mag = mag; max_bin = i; }
    }

    if (max_bin == 0) return 0.0f;
    return max_bin * freq_res * 0.5f;
}

// ============================================================
//  Lowpass filter (3-tap raised cosine, complex)
// ============================================================

void SignalIdentifier::lowpass_filter(std::vector<std::complex<float>>& data, float cutoff_hz) {
    const float taps[3] = { 0.25f, 0.5f, 0.25f };
    std::vector<std::complex<float>> result(data.size());
    for (unsigned int i = 0; i < data.size(); i++) {
        std::complex<float> sum(0.0f, 0.0f);
        for (int t = 0; t < 3; t++) {
            int idx = static_cast<int>(i) + t - 1;
            if (idx >= 0 && idx < static_cast<int>(data.size())) {
                sum += data[static_cast<unsigned int>(idx)] * taps[t];
            }
        }
        result[i] = sum;
    }
    data.swap(result);
}

// ============================================================
//  Lowpass filter for real data
// ============================================================

void SignalIdentifier::lowpass_filter_real(std::vector<float>& data, float cutoff_hz) {
    const float taps[3] = { 0.25f, 0.5f, 0.25f };
    std::vector<float> result(data.size());
    for (unsigned int i = 0; i < data.size(); i++) {
        float sum = 0.0f;
        for (int t = 0; t < 3; t++) {
            int idx = static_cast<int>(i) + t - 1;
            if (idx >= 0 && idx < static_cast<int>(data.size())) {
                sum += data[static_cast<unsigned int>(idx)] * taps[t];
            }
        }
        result[i] = sum;
    }
    data.swap(result);
}

// ============================================================
//  Symbol rate estimation (complex baseband - delay multiply)
// ============================================================

float SignalIdentifier::estimate_symbol_rate(const std::complex<float>* iq_data, unsigned int n) {
    if (n < 128) return 0.0f;

    // Delay-multiply: s(n) * conj(s(n-1)) produces tone at symbol rate
    unsigned int fft_size = 4096;
    unsigned int temp = 1;
    while (temp < fft_size && temp < n - 1) temp *= 2;
    fft_size = temp;
    if (fft_size > n - 1) fft_size = n - 1;
    if (fft_size < 64) fft_size = 64;

    FFT fft(fft_size, 1);
    std::vector<std::complex<float>> buf(fft_size);
    float freq_res = m_sample_rate / fft_size;

    // Delay-multiply with windowing
    std::complex<float> mean(0.0f, 0.0f);
    for (unsigned int i = 0; i < fft_size; i++) {
        float w = 0.5f * (1.0f - std::cos(2.0f * PI * i / (fft_size - 1)));
        buf[i] = iq_data[i + 1] * std::conj(iq_data[i]) * w;
        mean += buf[i];
    }
    mean /= static_cast<float>(fft_size);
    for (unsigned int i = 0; i < fft_size; i++) {
        buf[i] -= mean;
    }

    fft.execute(buf.data(), buf.data());

    float max_val = 0.0f;
    unsigned int max_bin = 0;
    // Search from bin 3 to skip DC leakage, up to fs/2
    for (unsigned int i = 3; i < fft_size / 2; i++) {
        float mag2 = buf[i].real() * buf[i].real() + buf[i].imag() * buf[i].imag();
        if (mag2 > max_val) { max_val = mag2; max_bin = i; }
    }

    if (max_bin == 0) return 0.0f;

    // Parabolic interpolation
    float refined_bin = static_cast<float>(max_bin);
    if (max_bin > 2 && max_bin < fft_size / 2 - 1) {
        float a = std::abs(buf[max_bin - 1]);
        float b = std::abs(buf[max_bin]);
        float c = std::abs(buf[max_bin + 1]);
        float denom = a - 2.0f * b + c;
        if (std::abs(denom) > 0.01f) {
            float p = 0.5f * (a - c) / denom;
            if (std::abs(p) < 1.0f) refined_bin += p;
        }
    }

    return refined_bin * freq_res;
}

// ============================================================
//  Symbol rate estimation for real signals (delay multiply)
// ============================================================

float SignalIdentifier::estimate_symbol_rate_real(const float* data, unsigned int n) {
    if (n < 128) return 0.0f;

    // For real signals, use absolute difference |s(n) - s(n-1)|
    // which has spectral peaks at harmonics of the symbol rate
    unsigned int fft_size = 4096;
    unsigned int temp = 1;
    while (temp < fft_size && temp < n - 1) temp *= 2;
    fft_size = temp;
    if (fft_size > n - 1) fft_size = n - 1;
    if (fft_size < 64) fft_size = 64;

    FFT fft(fft_size, 1);
    std::vector<std::complex<float>> buf(fft_size);
    float freq_res = m_sample_rate / fft_size;

    // Use squared difference: (s(n) - s(n-1))^2
    // For BPSK at symbol transitions, this produces pulses at symbol rate
    float mean = 0.0f;
    for (unsigned int i = 0; i < fft_size; i++) {
        float w = 0.5f * (1.0f - std::cos(2.0f * PI * i / (fft_size - 1)));
        float diff = data[i + 1] - data[i];
        buf[i] = std::complex<float>(diff * diff * w, 0.0f);
        mean += diff * diff * w;
    }
    mean /= static_cast<float>(fft_size);
    for (unsigned int i = 0; i < fft_size; i++) {
        buf[i] -= std::complex<float>(mean, 0.0f);
    }

    fft.execute(buf.data(), buf.data());

    float max_val = 0.0f;
    unsigned int max_bin = 0;
    for (unsigned int i = 3; i < fft_size / 2; i++) {
        float mag2 = buf[i].real() * buf[i].real() + buf[i].imag() * buf[i].imag();
        if (mag2 > max_val) { max_val = mag2; max_bin = i; }
    }

    if (max_bin == 0) return 0.0f;

    // Parabolic interpolation
    float refined_bin = static_cast<float>(max_bin);
    if (max_bin > 2 && max_bin < fft_size / 2 - 1) {
        float a = std::abs(buf[max_bin - 1]);
        float b = std::abs(buf[max_bin]);
        float c = std::abs(buf[max_bin + 1]);
        float denom = a - 2.0f * b + c;
        if (std::abs(denom) > 0.01f) {
            float p = 0.5f * (a - c) / denom;
            if (std::abs(p) < 1.0f) refined_bin += p;
        }
    }

    return refined_bin * freq_res;
}

// ============================================================
//  Modulation classification
// ============================================================

int SignalIdentifier::classify_modulation(const std::complex<float>* iq_data, unsigned int n) {
    float power = 0.0f;
    for (unsigned int i = 0; i < n; i++) {
        power += iq_data[i].real() * iq_data[i].real() + iq_data[i].imag() * iq_data[i].imag();
    }
    power /= n;
    float scale = 1.0f / std::sqrt(power + 1e-10f);

    // Circularity: |E[z^2]|
    std::complex<float> z2_sum(0.0f, 0.0f);
    for (unsigned int i = 0; i < n; i++) {
        std::complex<float> z = iq_data[i] * scale;
        z2_sum += z * z;
    }
    float circularity = std::abs(z2_sum) / static_cast<float>(n);

    float mean_amp = 0.0f;
    for (unsigned int i = 0; i < n; i++) {
        mean_amp += std::abs(iq_data[i]) * scale;
    }
    mean_amp /= n;

    float var_amp = 0.0f;
    for (unsigned int i = 0; i < n; i++) {
        float a = std::abs(iq_data[i]) * scale;
        float diff = a - mean_amp;
        var_amp += diff * diff;
    }
    var_amp /= n;

    float m2 = 0.0f, m4 = 0.0f;
    for (unsigned int i = 0; i < n; i++) {
        float a = std::abs(iq_data[i]) * scale;
        m2 += a * a;
        m4 += a * a * a * a;
    }
    m2 /= n;
    m4 /= n;
    float kurtosis = m4 / (m2 * m2 + 1e-20f);

    float var_freq = 0.0f;
    float mean_freq = 0.0f;
    for (unsigned int i = 1; i < n; i++) {
        std::complex<float> prod = iq_data[i] * std::conj(iq_data[i - 1]);
        mean_freq += std::arg(prod);
    }
    mean_freq /= (n - 1);
    for (unsigned int i = 1; i < n; i++) {
        std::complex<float> prod = iq_data[i] * std::conj(iq_data[i - 1]);
        float diff = std::arg(prod) - mean_freq;
        var_freq += diff * diff;
    }
    var_freq /= (n - 1);

    // Decision tree
    if (var_freq > 0.5f && circularity < 0.1f) return LIQUID_MODEM_FSK;
    if (circularity > 0.3f) return LIQUID_MODEM_BPSK;
    if (var_amp < 0.05f) return LIQUID_MODEM_QPSK;
    if (var_amp < 0.2f) {
        if (kurtosis > 1.8f) return LIQUID_MODEM_16QAM;
        else return LIQUID_MODEM_64QAM;
    }
    return LIQUID_MODEM_16QAM;
}

// ============================================================
//  SNR estimation
// ============================================================

float SignalIdentifier::estimate_snr(const std::complex<float>* iq_data, unsigned int n) {
    unsigned int fft_size = 1024;
    unsigned int temp = 1;
    while (temp < fft_size && temp < n) temp *= 2;
    fft_size = temp;
    if (fft_size > n) fft_size = n;

    FFT fft(fft_size, 1);
    std::vector<std::complex<float>> buf(fft_size);
    for (unsigned int i = 0; i < fft_size; i++) buf[i] = iq_data[i];
    fft.execute(buf.data(), buf.data());

    float max_mag = 0.0f;
    for (unsigned int i = 0; i < fft_size; i++) {
        float mag = std::abs(buf[i]);
        if (mag > max_mag) max_mag = mag;
    }

    float threshold = max_mag * 0.1f;
    float sig_pow = 0.0f, noise_pow = 0.0f;
    unsigned int sig_cnt = 0, noise_cnt = 0;
    for (unsigned int i = 0; i < fft_size; i++) {
        float mag2 = buf[i].real() * buf[i].real() + buf[i].imag() * buf[i].imag();
        if (std::abs(buf[i]) > threshold) { sig_pow += mag2; sig_cnt++; }
        else { noise_pow += mag2; noise_cnt++; }
    }
    if (noise_cnt == 0 || sig_cnt == 0) return 0.0f;
    return 10.0f * std::log10((sig_pow / sig_cnt) / (noise_pow / noise_cnt + 1e-20f));
}

// ============================================================
//  Helpers
// ============================================================

int SignalIdentifier::get_mod_order(int mod_type) {
    switch (mod_type) {
        case LIQUID_MODEM_BPSK:   return 2;
        case LIQUID_MODEM_QPSK:   return 4;
        case LIQUID_MODEM_8PSK:   return 8;
        case LIQUID_MODEM_16QAM:  return 16;
        case LIQUID_MODEM_32QAM:  return 32;
        case LIQUID_MODEM_64QAM:  return 64;
        case LIQUID_MODEM_256QAM: return 256;
        case LIQUID_MODEM_FSK:    return 2;
        case LIQUID_MODEM_GMSK:   return 2;
        default: return 0;
    }
}

int SignalIdentifier::get_bits_per_symbol(int mod_type) {
    switch (mod_type) {
        case LIQUID_MODEM_BPSK:   return 1;
        case LIQUID_MODEM_QPSK:   return 2;
        case LIQUID_MODEM_8PSK:   return 3;
        case LIQUID_MODEM_16QAM:  return 4;
        case LIQUID_MODEM_32QAM:  return 5;
        case LIQUID_MODEM_64QAM:  return 6;
        case LIQUID_MODEM_256QAM: return 8;
        case LIQUID_MODEM_FSK:    return 1;
        case LIQUID_MODEM_GMSK:   return 1;
        default: return 0;
    }
}

int SignalIdentifier::get_confidence(const std::complex<float>* iq_data, unsigned int n,
                                      int mod_type) {
    // Simple confidence metric based on how well the signal matches the classification
    float power = 0.0f;
    for (unsigned int i = 0; i < n; i++) {
        power += iq_data[i].real() * iq_data[i].real() + iq_data[i].imag() * iq_data[i].imag();
    }
    power /= n;
    float scale = 1.0f / std::sqrt(power + 1e-10f);

    // Compute circularity
    std::complex<float> z2_sum(0.0f, 0.0f);
    for (unsigned int i = 0; i < n; i++) {
        std::complex<float> z = iq_data[i] * scale;
        z2_sum += z * z;
    }
    float circularity = std::abs(z2_sum) / static_cast<float>(n);

    // Compute amplitude variance
    float mean_amp = 0.0f;
    for (unsigned int i = 0; i < n; i++)
        mean_amp += std::abs(iq_data[i]) * scale;
    mean_amp /= n;
    float var_amp = 0.0f;
    for (unsigned int i = 0; i < n; i++) {
        float a = std::abs(iq_data[i]) * scale;
        var_amp += (a - mean_amp) * (a - mean_amp);
    }
    var_amp /= n;

    int confidence = 50;  // base

    switch (mod_type) {
        case LIQUID_MODEM_BPSK:
            // Higher circularity → higher confidence
            confidence = static_cast<int>(50 + circularity * 50);
            break;
        case LIQUID_MODEM_QPSK:
            // Low circularity + low amplitude variance → high confidence
            confidence = static_cast<int>(50 + (1.0f - circularity) * 25 + (1.0f - var_amp * 20) * 25);
            break;
        case LIQUID_MODEM_16QAM:
        case LIQUID_MODEM_64QAM:
        case LIQUID_MODEM_256QAM:
            // Moderate amplitude variance expected
            confidence = static_cast<int>(50 + (1.0f - std::abs(var_amp - 0.1f) * 10) * 50);
            break;
        default:
            confidence = 50;
            break;
    }

    if (confidence < 0) confidence = 0;
    if (confidence > 100) confidence = 100;
    return confidence;
}

} // namespace liquid
