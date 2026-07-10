#pragma once
#include <complex>

namespace liquid {

class Agc {
public:
    Agc(float bt = 0.01f, float beta = 0.15f, float signal_level = 1.0f);
    ~Agc();

    void execute(std::complex<float>& x);
    void execute(float& x);
    void execute_block(std::complex<float>* x, unsigned int n);
    
    void set_bandwidth(float bt);
    float get_bandwidth() const { return m_bt; }
    void set_signal_level(float level);
    float get_signal_level() const { return m_signal_level; }
    float get_rssi() const { return m_rssi; }
    void reset();

private:
    float m_bt;
    float m_beta;
    float m_signal_level;
    float m_rssi;
    float m_alpha;
};

} // namespace liquid
