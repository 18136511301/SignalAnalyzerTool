#include "liquid_dll.h"
#include "core/error.h"
#include "fft/fft.h"
#include "filter/firfilt.h"
#include "filter/firdes.h"
#include "nco/nco.h"
#include "modem/modem.h"
#include "agc/agc.h"
#include "random/random.h"
#include "signal/identify.h"
#include "signal/demod.h"

using namespace liquid;

// ==================== FFT ====================
LIQUID_API void* liquid_fft_create(unsigned int nfft) {
    try { return new FFT(nfft, 1); }
    catch (...) { return nullptr; }
}

LIQUID_API void liquid_fft_destroy(void* q) {
    delete static_cast<FFT*>(q);
}

LIQUID_API int liquid_fft_execute_forward(void* q, ComplexFloat* x, ComplexFloat* y) {
    if (!q) return LIQUID_EIOBJ;
    static_cast<FFT*>(q)->execute(
        reinterpret_cast<std::complex<float>*>(x),
        reinterpret_cast<std::complex<float>*>(y));
    return LIQUID_OK;
}

LIQUID_API int liquid_fft_execute_inverse(void* q, ComplexFloat* x, ComplexFloat* y) {
    if (!q) return LIQUID_EIOBJ;
    FFT* fft = static_cast<FFT*>(q);
    FFT inv_fft(fft->size(), -1);
    inv_fft.execute(
        reinterpret_cast<std::complex<float>*>(x),
        reinterpret_cast<std::complex<float>*>(y));
    return LIQUID_OK;
}

// ==================== FIR Filter ====================
LIQUID_API void* liquid_firfilt_create_kaiser(unsigned int n, float fc, float As) {
    try {
        auto taps = FirDes::design_kaiser_lowpass(n, fc, As);
        return new FirFilter(taps);
    } catch (...) { return nullptr; }
}

LIQUID_API void* liquid_firfilt_create(const float* taps, unsigned int n) {
    try { return new FirFilter(taps, n); }
    catch (...) { return nullptr; }
}

LIQUID_API void liquid_firfilt_destroy(void* q) {
    delete static_cast<FirFilter*>(q);
}

LIQUID_API int liquid_firfilt_execute_cf32(void* q, ComplexFloat* x, ComplexFloat* y) {
    if (!q) return LIQUID_EIOBJ;
    static_cast<FirFilter*>(q)->execute(
        *reinterpret_cast<std::complex<float>*>(x),
        *reinterpret_cast<std::complex<float>*>(y));
    return LIQUID_OK;
}

LIQUID_API int liquid_firfilt_execute_rf32(void* q, float x, float* y) {
    if (!q) return LIQUID_EIOBJ;
    static_cast<FirFilter*>(q)->execute(x, *y);
    return LIQUID_OK;
}

LIQUID_API void liquid_firfilt_reset(void* q) {
    if (q) static_cast<FirFilter*>(q)->reset();
}

// ==================== NCO ====================
LIQUID_API void* liquid_nco_create(int type) {
    try { return new Nco(static_cast<NcoType>(type)); }
    catch (...) { return nullptr; }
}

LIQUID_API void liquid_nco_destroy(void* q) {
    delete static_cast<Nco*>(q);
}

LIQUID_API void liquid_nco_set_frequency(void* q, float dtheta) {
    if (q) static_cast<Nco*>(q)->set_frequency(dtheta);
}

LIQUID_API float liquid_nco_get_frequency(void* q) {
    return q ? static_cast<Nco*>(q)->get_frequency() : 0.0f;
}

LIQUID_API void liquid_nco_execute(void* q, ComplexFloat* y) {
    if (q) static_cast<Nco*>(q)->execute(*reinterpret_cast<std::complex<float>*>(y));
}

LIQUID_API void liquid_nco_mix(void* q, ComplexFloat x, ComplexFloat* y) {
    if (q) static_cast<Nco*>(q)->mix(
        *reinterpret_cast<std::complex<float>*>(&x),
        *reinterpret_cast<std::complex<float>*>(y));
}

// ==================== Modem ====================
LIQUID_API void* liquid_modem_create(int type) {
    try { return new Modem(static_cast<ModulationType>(type)); }
    catch (...) { return nullptr; }
}

LIQUID_API void liquid_modem_destroy(void* q) {
    delete static_cast<Modem*>(q);
}

LIQUID_API int liquid_modem_modulate(void* q, unsigned int symbol_in, ComplexFloat* symbol_out) {
    if (!q) return LIQUID_EIOBJ;
    static_cast<Modem*>(q)->modulate(symbol_in,
        *reinterpret_cast<std::complex<float>*>(symbol_out));
    return LIQUID_OK;
}

LIQUID_API int liquid_modem_demodulate(void* q, ComplexFloat symbol_in, unsigned int* symbol_out) {
    if (!q) return LIQUID_EIOBJ;
    static_cast<Modem*>(q)->demodulate(
        *reinterpret_cast<std::complex<float>*>(&symbol_in), *symbol_out);
    return LIQUID_OK;
}

LIQUID_API unsigned int liquid_modem_order(void* q) {
    return q ? static_cast<Modem*>(q)->order() : 0;
}

// ==================== AGC ====================
LIQUID_API void* liquid_agc_create(float bt, float signal_level) {
    try { return new Agc(bt, 0.15f, signal_level); }
    catch (...) { return nullptr; }
}

LIQUID_API void liquid_agc_destroy(void* q) {
    delete static_cast<Agc*>(q);
}

LIQUID_API void liquid_agc_execute(void* q, ComplexFloat* x) {
    if (q) static_cast<Agc*>(q)->execute(*reinterpret_cast<std::complex<float>*>(x));
}

LIQUID_API float liquid_agc_get_rssi(void* q) {
    return q ? static_cast<Agc*>(q)->get_rssi() : 0.0f;
}

LIQUID_API void liquid_agc_reset(void* q) {
    if (q) static_cast<Agc*>(q)->reset();
}

// ==================== Random ====================
LIQUID_API void* liquid_rand_create(unsigned int seed) {
    try { return new Random(seed); }
    catch (...) { return nullptr; }
}

LIQUID_API void liquid_rand_destroy(void* q) {
    delete static_cast<Random*>(q);
}

LIQUID_API float liquid_randf(void* q) {
    return q ? static_cast<Random*>(q)->randf() : 0.0f;
}

LIQUID_API float liquid_randn(void* q) {
    return q ? static_cast<Random*>(q)->randn() : 0.0f;
}

// ==================== Signal Processing ====================
LIQUID_API int liquid_signal_identify_cf32(
    const ComplexFloat* iq_data, unsigned int num_samples,
    float sample_rate, SignalInfo* info) {
    try {
        SignalIdentifier id(sample_rate);
        SignalInfo result = id.identify(
            reinterpret_cast<const std::complex<float>*>(iq_data), num_samples);
        *info = result;
        return LIQUID_OK;
    } catch (...) { return LIQUID_EINT; }
}

LIQUID_API int liquid_signal_identify_rf32(
    const float* iq_data, unsigned int num_samples,
    float sample_rate, SignalInfo* info) {
    try {
        SignalIdentifier id(sample_rate);
        SignalInfo result = id.identify(iq_data, num_samples, false);
        *info = result;
        return LIQUID_OK;
    } catch (...) { return LIQUID_EINT; }
}

LIQUID_API void* liquid_demod_create(int modulation_type, float symbol_rate, float sample_rate) {
    try { return new Demodulator(static_cast<ModulationType>(modulation_type), symbol_rate, sample_rate); }
    catch (...) { return nullptr; }
}

LIQUID_API void liquid_demod_destroy(void* q) {
    delete static_cast<Demodulator*>(q);
}

LIQUID_API int liquid_demod_execute(void* q, const ComplexFloat* iq_data, 
                                    unsigned int n, unsigned int* symbols, unsigned int* num_symbols) {
    if (!q) return LIQUID_EIOBJ;
    return static_cast<Demodulator*>(q)->execute(
        reinterpret_cast<const std::complex<float>*>(iq_data), n, symbols, *num_symbols);
}

LIQUID_API void liquid_demod_set_center_freq(void* q, float freq_hz) {
    if (q) static_cast<Demodulator*>(q)->set_center_freq(freq_hz);
}

LIQUID_API void liquid_demod_get_soft_symbols(void* q, ComplexFloat* buffer, unsigned int* n) {
    if (q && buffer && n)
        static_cast<Demodulator*>(q)->get_soft_symbols(
            reinterpret_cast<std::complex<float>*>(buffer), *n);
}

// ==================== V2 API ====================
LIQUID_API int liquid_signal_identify_v2(
    const IdentifyParams* params,
    IdentifyResult* result) {
    try {
        if (!params || !result) return LIQUID_EIOBJ;
        SignalIdentifier id(params->sample_rate);
        return id.identify_v2(params, result);
    } catch (...) { return LIQUID_EINT; }
}

LIQUID_API int liquid_demodulate_v2(
    const DemodInput* input,
    DemodOutput* output) {
    try {
        if (!input || !output) return LIQUID_EIOBJ;
        Demodulator demod(static_cast<ModulationType>(input->modulation_type),
                          input->symbol_rate, input->sample_rate);
        return demod.demodulate_v2(input, output);
    } catch (...) { return LIQUID_EINT; }
}

// ==================== Utilities ====================
LIQUID_API const char* liquid_version(void) {
    return "liquid-dsp-cpp 1.0.0";
}

LIQUID_API const char* liquid_error_string(int code) {
    return ::liquid::core::error_string(static_cast<::liquid::ErrorCode>(code));
}
