#pragma once
#include <vector>
#include <complex>
#include <cstring>

namespace liquid {

class FirFilter {
public:
    FirFilter(const std::vector<float>& taps);
    FirFilter(const float* taps, unsigned int n);
    ~FirFilter();

    void execute(const std::complex<float>& x, std::complex<float>& y);
    void execute_block(const std::complex<float>* x, unsigned int n, std::complex<float>* y);
    
    void execute(float x, float& y);
    void execute_block(const float* x, unsigned int n, float* y);
    
    unsigned int group_delay() const { return m_taps.size() / 2; }
    void reset();

private:
    std::vector<float> m_taps;
    std::vector<std::complex<float>> m_buffer;
    unsigned int m_index;
};

} // namespace liquid
