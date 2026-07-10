#pragma once
#include <complex>

namespace liquid {

enum NcoType {
    NCO_SINUSOID = 0,
    NCO_COMPLEX = 1
};

class Nco {
public:
    Nco(NcoType type = NCO_SINUSOID);
    ~Nco();

    void set_frequency(float dtheta);
    float get_frequency() const { return m_dtheta; }
    
    void set_phase(float theta);
    float get_phase() const { return m_theta; }
    void reset_phase() { m_theta = 0.0f; }
    
    void execute(float& s);
    void execute(std::complex<float>& c);
    void execute_block(std::complex<float>* c, unsigned int n);
    
    void mix(float x, float& y);
    void mix(std::complex<float> x, std::complex<float>& y);
    void mix_block(std::complex<float>* x, unsigned int n);

private:
    NcoType m_type;
    float m_dtheta;
    float m_theta;
    
    static const float PI;
};

} // namespace liquid
