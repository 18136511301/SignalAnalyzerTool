#pragma once
#include <complex>

namespace liquid {

// Complex number type for DLL interface (C-compatible)
struct ComplexFloat {
    float real;   // Real part
    float imag;   // Imaginary part

    ComplexFloat() : real(0), imag(0) {}
    ComplexFloat(float r, float i) : real(r), imag(i) {}
    ComplexFloat(std::complex<float> c) : real(c.real()), imag(c.imag()) {}

    // Implicit conversion to std::complex<float>
    operator std::complex<float>() const { return std::complex<float>(real, imag); }
};

// Double precision complex (reserved)
struct ComplexDouble {
    double real;
    double imag;

    ComplexDouble() : real(0), imag(0) {}
    ComplexDouble(double r, double i) : real(r), imag(i) {}
};

// Error codes returned by all API functions
enum ErrorCode {
    LIQUID_OK = 0,         // Success
    LIQUID_EINT = 1,       // Internal error
    LIQUID_EIOBJ = 2,      // Invalid object (null pointer etc.)
    LIQUID_EICONFIG = 3,   // Invalid configuration
    LIQUID_EIVAL = 4,      // Invalid value
    LIQUID_EIRANGE = 5,    // Out of range
    LIQUID_EIMODE = 6,     // Invalid mode
    LIQUID_EUMODE = 7,     // Unsupported mode
    LIQUID_ENOINIT = 8,    // Object not initialized
    LIQUID_EIMEM = 9,      // Memory error
    LIQUID_EIO = 10,       // I/O error
    LIQUID_ENOCONV = 11,   // Algorithm did not converge
    LIQUID_ENOIMP = 12     // Not implemented
};

// Modulation type enumeration
enum ModulationType {
    // PSK (Phase Shift Keying)
    LIQUID_MODEM_BPSK = 0,    // Binary PSK (1 bit/symbol)
    LIQUID_MODEM_QPSK = 1,    // Quadrature PSK (2 bits/symbol)
    LIQUID_MODEM_8PSK = 2,    // 8-PSK (3 bits/symbol)
    LIQUID_MODEM_16PSK = 3,   // 16-PSK (4 bits/symbol)
    LIQUID_MODEM_32PSK = 4,   // 32-PSK
    LIQUID_MODEM_64PSK = 5,   // 64-PSK
    LIQUID_MODEM_128PSK = 6,  // 128-PSK
    LIQUID_MODEM_256PSK = 7,  // 256-PSK

    // QAM (Quadrature Amplitude Modulation)
    LIQUID_MODEM_2QAM = 8,    // 2-QAM
    LIQUID_MODEM_4QAM = 9,    // 4-QAM (same as QPSK)
    LIQUID_MODEM_8QAM = 10,   // 8-QAM
    LIQUID_MODEM_16QAM = 11,  // 16-QAM (4 bits/symbol)
    LIQUID_MODEM_32QAM = 12,  // 32-QAM
    LIQUID_MODEM_64QAM = 13,  // 64-QAM (6 bits/symbol)
    LIQUID_MODEM_128QAM = 14, // 128-QAM
    LIQUID_MODEM_256QAM = 15, // 256-QAM (8 bits/symbol)

    // General types
    LIQUID_MODEM_ASK = 16,    // Amplitude Shift Keying
    LIQUID_MODEM_PSK = 17,    // Generic PSK
    LIQUID_MODEM_QAM = 18,    // Generic QAM
    LIQUID_MODEM_APSK = 19,   // Amplitude Phase Shift Keying
    LIQUID_MODEM_FSK = 20,    // Frequency Shift Keying
    LIQUID_MODEM_GMSK = 21    // Gaussian Minimum Shift Keying
};

// Signal identification result
struct SignalInfo {
    float center_freq;      // Estimated center frequency (Hz)
    float symbol_rate;      // Estimated symbol rate (symbols/s)
    float bandwidth;        // Estimated signal bandwidth (Hz)
    float snr;              // Estimated signal-to-noise ratio (dB)
    int   modulation_type;  // Modulation type (ModulationType enum)
    int   modulation_order; // Modulation order

    SignalInfo() : center_freq(0), symbol_rate(0), bandwidth(0),
                   snr(0), modulation_type(0), modulation_order(0) {}
};

// Signal type enumeration
enum SignalType {
    LIQUID_SIGNAL_REAL = 0,     // Real signal (mono, 1 channel)
    LIQUID_SIGNAL_COMPLEX = 1   // Complex IQ signal (I/Q interleaved)
};

// ==================== New V2 API Structures ====================

// Signal identification input parameters
struct IdentifyParams {
    const void* data;       // Raw binary data (int16_t or float, as-is from file)
    unsigned int data_len;  // Length of data in BYTES
    int bit_width;          // Sample bit width: 16 (int16), 32 (float)
    int signal_type;        // SignalType: REAL or COMPLEX
    float sample_rate;      // Sample rate in Hz

    IdentifyParams()
        : data(nullptr), data_len(0), bit_width(16),
          signal_type(LIQUID_SIGNAL_REAL), sample_rate(100000.0f) {}
};

// Signal identification output result (V2)
struct IdentifyResult {
    float center_freq;      // Estimated center frequency (Hz)
    float symbol_rate;      // Estimated symbol rate (symbols/s)
    float bandwidth;        // Estimated 3dB bandwidth (Hz)
    float snr;              // Estimated SNR (dB)
    int   modulation_type;  // ModulationType enum value
    int   modulation_order; // Constellation order (2, 4, 8, 16, ...)
    int   bits_per_symbol;  // Bits per symbol (1, 2, 3, 4, ...)
    int   confidence;       // Classification confidence (0-100)

    IdentifyResult()
        : center_freq(0), symbol_rate(0), bandwidth(0), snr(0),
          modulation_type(0), modulation_order(0), bits_per_symbol(0),
          confidence(0) {}
};

// Demodulation input parameters (V2)
struct DemodInput {
    const void* data;           // Raw binary data
    unsigned int data_len;      // Length of data in BYTES
    int bit_width;              // Sample bit width: 16 (int16), 32 (float)
    int signal_type;            // SignalType: REAL or COMPLEX
    float sample_rate;          // Sample rate (Hz)
    float carrier_freq;         // Carrier/center frequency (Hz)
    float symbol_rate;          // Modulation symbol rate (symbols/s)
    int   modulation_type;      // ModulationType enum value

    DemodInput()
        : data(nullptr), data_len(0), bit_width(16),
          signal_type(LIQUID_SIGNAL_REAL), sample_rate(100000.0f),
          carrier_freq(0), symbol_rate(10000),
          modulation_type(LIQUID_MODEM_BPSK) {}
};

// Demodulation output result (V2)
struct DemodOutput {
    unsigned int* hard_symbols;     // Hard-decision symbol indices (caller allocates)
    ComplexFloat* soft_symbols;     // Soft-decision I/Q before decision (caller allocates)
    unsigned int  hard_count;       // Number of hard symbols written
    unsigned int  soft_count;       // Number of soft symbols written
    unsigned int  hard_capacity;    // Max hard symbols the buffer can hold
    unsigned int  soft_capacity;    // Max soft symbols the buffer can hold
    bool          locked;           // PLL locked indicator
    float         freq_offset;      // Estimated residual frequency offset (Hz)

    DemodOutput()
        : hard_symbols(nullptr), soft_symbols(nullptr),
          hard_count(0), soft_count(0), hard_capacity(0), soft_capacity(0),
          locked(false), freq_offset(0) {}
};

} // namespace liquid
