#include "random.h"

namespace liquid {

Random::Random(unsigned int seed) 
    : m_gen(seed == 0 ? std::random_device{}() : seed),
      m_uniform(0.0f, 1.0f),
      m_normal(0.0f, 1.0f) {
}

Random::~Random() {
}

float Random::randf() {
    return m_uniform(m_gen);
}

float Random::randn() {
    return m_normal(m_gen);
}

std::complex<float> Random::randnf() {
    return std::complex<float>(m_normal(m_gen), m_normal(m_gen));
}

unsigned int Random::randui(unsigned int max) {
    std::uniform_int_distribution<unsigned int> dist(0, max - 1);
    return dist(m_gen);
}

} // namespace liquid
