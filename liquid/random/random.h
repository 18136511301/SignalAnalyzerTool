#pragma once
#include <complex>
#include <random>

namespace liquid {

class Random {
public:
    Random(unsigned int seed = 0);
    ~Random();

    float randf();
    float randn();
    std::complex<float> randnf();
    unsigned int randui(unsigned int max);

private:
    std::mt19937 m_gen;
    std::uniform_real_distribution<float> m_uniform;
    std::normal_distribution<float> m_normal;
};

} // namespace liquid
