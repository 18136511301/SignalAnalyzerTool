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
//  Main identify function (complex IQ data)
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

    // 2. Downconvert to baseband (coarse)
    float mix_freq = -info.center_freq;
    std::vector<std::complex<float>> baseband(n);
    for (unsigned int i = 0; i < n; i++) {
        float phase = 2.0f * PI * mix_freq * i / m_sample_rate;
        std::complex<float> nco_val(std::cos(phase), std::sin(phase));
        baseband[i] = data[i] * nco_val;
    }

    // 2a. Lowpass filter BEFORE refinement (critical for real signals:
    //     removes 4*fc aliases that create spurious peaks in squared spectrum)
    lowpass_filter(baseband, m_sample_rate * 0.2f);

    // 2b. Refine center frequency via squared-signal method (on filtered data)
    float residual = refine_frequency(baseband.data(), n, info.center_freq);
    info.center_freq += residual;

    // 2c. Re-downconvert with refined frequency
    if (std::abs(residual) > 0.5f) {
        mix_freq = -info.center_freq;
        for (unsigned int i = 0; i < n; i++) {
            float phase = 2.0f * PI * mix_freq * i / m_sample_rate;
            std::complex<float> nco_val(std::cos(phase), std::sin(phase));
            baseband[i] = data[i] * nco_val;
        }
        // Re-apply lowpass filter on re-downconverted data
        lowpass_filter(baseband, m_sample_rate * 0.2f);
    }

    // 3. Estimate bandwidth
    std::vector<float> bb_psd, bb_freq;
    compute_psd(baseband.data(), n, bb_psd, bb_freq);
    info.bandwidth = estimate_bandwidth(bb_psd, bb_freq);

    // 4. Estimate symbol rate
    info.symbol_rate = estimate_symbol_rate(baseband.data(), n);

    // 5. Classify modulation
    info.modulation_type = classify_modulation(baseband.data(), n);
    info.modulation_order = get_mod_order(info.modulation_type);

    // 6. Estimate SNR
    info.snr = estimate_snr(data.data(), n);

    return info;
}

// ============================================================
//  Float data entry point
// ============================================================

SignalInfo SignalIdentifier::identify(const float* iq_data, unsigned int n, bool is_complex) {
    if (is_complex) {
        unsigned int num_samples = n / 2;
        std::vector<std::complex<float>> complex_data(num_samples);
        for (unsigned int i = 0; i < num_samples; i++) {
            complex_data[i] = std::complex<float>(iq_data[2 * i], iq_data[2 * i + 1]);
        }
        return identify(complex_data.data(), num_samples);
    }

    // ============================================================
    //  Real signal path (self-contained)
    // ============================================================
    SignalInfo info;
    if (n < 64) return info;

    // 1) Find carrier via squared-signal method:
    //    s(t)^2 = A^2/2 + A^2/2*cos(4*pi*fc*t)  -->  strong tone at 2*fc
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

        // Search for 2*fc tone in [N/3, N/2] range (avoids DC & Nyquist edge)
        unsigned int lo = fft_size / 3;
        unsigned int hi = fft_size / 2;
        if (hi >= fft_size) hi = fft_size - 1;
        float max_mag = 0.0f;
        unsigned int max_bin = 0;
        for (unsigned int i = lo; i <= hi; i++) {
            float mag = std::abs(buf[i]);
            if (mag > max_mag) { max_mag = mag; max_bin = i; }
        }
        if (max_bin > 0) {
            float freq_res = m_sample_rate / fft_size;
            float peak_freq = max_bin * freq_res;
            if (max_bin > 0 && max_bin < fft_size - 1) {
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

    // 3) Lowpass filter (removes 2*fc image)
    lowpass_filter(baseband, m_sample_rate * 0.2f);

    // 3b) Refine frequency via squared baseband (DC subtracted to reveal near-DC tone)
    {
        FFT fft_sq(2048, 1);
        std::vector<std::complex<float>> sq_buf(2048);
        unsigned int sq_n = (n < 2048) ? n : 2048;
        for (unsigned int i = 0; i < sq_n; i++) {
            float w = 0.5f * (1.0f - std::cos(2.0f * PI * i / (sq_n - 1)));
            sq_buf[i] = baseband[i] * baseband[i] * w;
        }
        // Subtract mean to expose 2*residual_freq tone near DC
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
                // Re-downconvert with refined frequency
                float new_mix = -info.center_freq;
                for (unsigned int i = 0; i < n; i++) {
                    float phase = 2.0f * PI * new_mix * i / m_sample_rate;
                    baseband[i] = std::complex<float>(iq_data[i], 0.0f) *
                                   std::complex<float>(std::cos(phase), std::sin(phase));
                }
                lowpass_filter(baseband, m_sample_rate * 0.2f);
            }
        }
    }

    // 4) Bandwidth
    {
        std::vector<float> bb_psd, bb_freq;
        compute_psd(baseband.data(), n, bb_psd, bb_freq);
        info.bandwidth = estimate_bandwidth(bb_psd, bb_freq);
    }

    // 5) Symbol rate
    info.symbol_rate = estimate_symbol_rate(baseband.data(), n);

    // 6) Modulation (on 2000-sample segment to limit residual-rotation smearing)
    unsigned int classify_n = (n < 2000) ? n : 2000;
    info.modulation_type = classify_modulation(baseband.data(), classify_n);
    info.modulation_order = get_mod_order(info.modulation_type);

    // 7) SNR
    info.snr = estimate_snr(baseband.data(), n);

    return info;
}

// ============================================================
//  PSD computation
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
//  Bandwidth estimation
// ============================================================

float SignalIdentifier::estimate_bandwidth(const std::vector<float>& psd,
                                            const std::vector<float>& freq_axis) {
    // Convert PSD from dB to linear power and compute total
    double total_power = 0.0;
    for (unsigned int i = 0; i < psd.size(); i++) {
        total_power += std::pow(10.0, psd[i] / 10.0);
    }

    if (total_power <= 0.0) return 0.0f;

    // Find the peak index
    float max_val = -1e10f;
    unsigned int max_idx = psd.size() / 2;
    for (unsigned int i = 0; i < psd.size(); i++) {
        if (psd[i] > max_val) { max_val = psd[i]; max_idx = i; }
    }

    // Find occupied bandwidth (99% power around the peak)
    double target_power = total_power * 0.99;
    double cum_power = 0.0;

    // Expand outward from the peak until we accumulate target_power
    int left_idx = static_cast<int>(max_idx);
    int right_idx = static_cast<int>(max_idx);
    double left_val = std::pow(10.0, psd[max_idx] / 10.0);
    double right_val = left_val;
    cum_power = left_val;  // start with peak bin

    while (cum_power < target_power) {
        bool moved = false;

        // Try to expand left
        if (left_idx > 0) {
            double lv = std::pow(10.0, psd[left_idx - 1] / 10.0);
            left_idx--;
            cum_power += lv;
            left_val = lv;
            moved = true;
        }
        // Try to expand right
        if (right_idx < static_cast<int>(psd.size()) - 1) {
            double rv = std::pow(10.0, psd[right_idx + 1] / 10.0);
            right_idx++;
            cum_power += rv;
            right_val = rv;
            moved = true;
        }

        if (!moved) break;  // reached both edges
    }

    if (right_idx <= left_idx) return 0.0f;
    return freq_axis[right_idx] - freq_axis[left_idx];
}

// ============================================================
//  Frequency refinement via squared-signal spectrum
//  For BPSK/QPSK, s(t)^2 produces a tone at 2*residual_offset
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

    // Square the baseband signal to expose 2*residual_freq tone
    std::complex<float> mean(0.0f, 0.0f);
    for (unsigned int i = 0; i < fft_size; i++) {
        float w = 0.5f * (1.0f - std::cos(2.0f * PI * i / (fft_size - 1)));
        buf[i] = iq_data[i] * iq_data[i] * w;
        mean += buf[i];
    }
    mean /= static_cast<float>(fft_size);
    // Subtract DC to reveal near-DC tone from residual carrier offset
    for (unsigned int i = 0; i < fft_size; i++) {
        buf[i] -= mean;
    }
    fft.execute(buf.data(), buf.data());

    // Find peak near DC (bin 1 to N/32 ≈ 0~3.1kHz at 100kHz)
    unsigned int search_limit = fft_size / 32;
    if (search_limit < 4) search_limit = 4;
    float max_mag = 0.0f;
    unsigned int max_bin = 0;
    for (unsigned int i = 1; i < search_limit; i++) {
        float mag = std::abs(buf[i]);
        if (mag > max_mag) { max_mag = mag; max_bin = i; }
    }

    if (max_bin == 0) return 0.0f;

    // Peak is at 2 * residual_offset
    return max_bin * freq_res * 0.5f;
}

// ============================================================
//  Simple lowpass filter (moving average, in-place on complex)
//  Removes the 2*fc image after downconversion
// ============================================================

void SignalIdentifier::lowpass_filter(std::vector<std::complex<float>>& data, float cutoff_hz) {
    // 3-tap raised cosine: H(f) = cos^2(pi*f/fs)
    // 3dB cutoff at fs/4, -32dB at 45kHz for fs=100kHz
    // Much sharper than moving average of same length
    const float taps[3] = { 0.25f, 0.5f, 0.25f };

    std::vector<std::complex<float>> result(data.size());
    for (unsigned int i = 0; i < data.size(); i++) {
        std::complex<float> sum(0.0f, 0.0f);
        for (int t = 0; t < 3; t++) {
            int idx = static_cast<int>(i) + t - 1;  // centered (group delay = 1)
            if (idx >= 0 && idx < static_cast<int>(data.size())) {
                sum += data[static_cast<unsigned int>(idx)] * taps[t];
            }
        }
        result[i] = sum;
    }
    data.swap(result);
}

// ============================================================
//  Symbol rate estimation
// ============================================================

float SignalIdentifier::estimate_symbol_rate(const std::complex<float>* iq_data, unsigned int n) {
    unsigned int fft_size = 4096;
    unsigned int temp = 1;
    while (temp < fft_size && temp < n) temp *= 2;
    fft_size = temp;
    if (fft_size > n) fft_size = n;

    FFT fft(fft_size, 1);
    std::vector<std::complex<float>> buf(fft_size);
    float freq_res = m_sample_rate / fft_size;

    // Square the baseband signal
    std::complex<float> mean(0.0f, 0.0f);
    for (unsigned int i = 0; i < fft_size; i++) {
        float w = 0.5f * (1.0f - std::cos(2.0f * PI * i / (fft_size - 1)));
        buf[i] = iq_data[i] * iq_data[i] * w;
        mean += buf[i];
    }
    mean /= static_cast<float>(fft_size);

    // Subtract DC to reveal symbol-rate peak near DC residual
    for (unsigned int i = 0; i < fft_size; i++) {
        buf[i] -= mean;
    }

    fft.execute(buf.data(), buf.data());

    float max_val = 0.0f;
    unsigned int max_bin = 0;
    // Start from bin 5 to skip residual DC leakage
    for (unsigned int i = 5; i < fft_size / 2; i++) {
        float mag2 = buf[i].real() * buf[i].real() + buf[i].imag() * buf[i].imag();
        if (mag2 > max_val) { max_val = mag2; max_bin = i; }
    }

    return max_bin * freq_res;
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

    // Circularity: |E[z^2]| where z is the normalized sample
    // For BPSK, z^2 averages to ~1 (all points on one axis)
    // For QPSK/QAM, z^2 averages to ~0 (points at 90 degrees)
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
    // 1) FSK: very high phase variance, low circularity
    if (var_freq > 0.5f && circularity < 0.1f) return LIQUID_MODEM_FSK;

    // 2) BPSK: high circularity (signal concentrated on one axis)
    if (circularity > 0.3f) return LIQUID_MODEM_BPSK;

    // 3) Constant amplitude -> PSK
    if (var_amp < 0.05f) return LIQUID_MODEM_QPSK;

    // 4) QAM variants
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

} // namespace liquid
