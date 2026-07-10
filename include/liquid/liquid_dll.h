#pragma once
// liquid-dsp C++ DLL Export Header
// See API文档.md for detailed usage

#include "liquid_api.h"
#include "core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

// FFT
LIQUID_API void* liquid_fft_create(unsigned int nfft);
LIQUID_API void  liquid_fft_destroy(void* q);
LIQUID_API int   liquid_fft_execute_forward(void* q, liquid::ComplexFloat* x, liquid::ComplexFloat* y);
LIQUID_API int   liquid_fft_execute_inverse(void* q, liquid::ComplexFloat* x, liquid::ComplexFloat* y);

// FIR Filter
LIQUID_API void* liquid_firfilt_create_kaiser(unsigned int n, float fc, float As);
LIQUID_API void* liquid_firfilt_create(const float* taps, unsigned int n);
LIQUID_API void  liquid_firfilt_destroy(void* q);
LIQUID_API int   liquid_firfilt_execute_cf32(void* q, liquid::ComplexFloat* x, liquid::ComplexFloat* y);
LIQUID_API int   liquid_firfilt_execute_rf32(void* q, float x, float* y);
LIQUID_API void  liquid_firfilt_reset(void* q);

// NCO
LIQUID_API void* liquid_nco_create(int type);
LIQUID_API void  liquid_nco_destroy(void* q);
LIQUID_API void  liquid_nco_set_frequency(void* q, float dtheta);
LIQUID_API float liquid_nco_get_frequency(void* q);
LIQUID_API void  liquid_nco_execute(void* q, liquid::ComplexFloat* y);
LIQUID_API void  liquid_nco_mix(void* q, liquid::ComplexFloat x, liquid::ComplexFloat* y);

// Modem
LIQUID_API void* liquid_modem_create(int type);
LIQUID_API void  liquid_modem_destroy(void* q);
LIQUID_API int   liquid_modem_modulate(void* q, unsigned int symbol_in, liquid::ComplexFloat* symbol_out);
LIQUID_API int   liquid_modem_demodulate(void* q, liquid::ComplexFloat symbol_in, unsigned int* symbol_out);
LIQUID_API unsigned int liquid_modem_order(void* q);

// AGC
LIQUID_API void* liquid_agc_create(float bt, float signal_level);
LIQUID_API void  liquid_agc_destroy(void* q);
LIQUID_API void  liquid_agc_execute(void* q, liquid::ComplexFloat* x);
LIQUID_API float liquid_agc_get_rssi(void* q);
LIQUID_API void  liquid_agc_reset(void* q);

// Random
LIQUID_API void* liquid_rand_create(unsigned int seed);
LIQUID_API void  liquid_rand_destroy(void* q);
LIQUID_API float liquid_randf(void* q);
LIQUID_API float liquid_randn(void* q);

// Signal Processing
LIQUID_API int liquid_signal_identify_cf32(
    const liquid::ComplexFloat* iq_data, unsigned int num_samples,
    float sample_rate, liquid::SignalInfo* info);
LIQUID_API int liquid_signal_identify_rf32(
    const float* iq_data, unsigned int num_samples,
    float sample_rate, liquid::SignalInfo* info);

// Demodulation
LIQUID_API void* liquid_demod_create(int modulation_type, float symbol_rate, float sample_rate);
LIQUID_API void  liquid_demod_destroy(void* q);
LIQUID_API int   liquid_demod_execute(void* q, const liquid::ComplexFloat* iq_data,
                                      unsigned int n, unsigned int* symbols, unsigned int* num_symbols);
LIQUID_API void  liquid_demod_set_center_freq(void* q, float freq_hz);
LIQUID_API void  liquid_demod_get_soft_symbols(void* q, liquid::ComplexFloat* buffer, unsigned int* n);

// Utilities
LIQUID_API const char* liquid_version(void);
LIQUID_API const char* liquid_error_string(int code);

#ifdef __cplusplus
}
#endif
