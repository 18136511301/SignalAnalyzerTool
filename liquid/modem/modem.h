#pragma once
#include "../core/types.h"
#include <complex>
#include <vector>

namespace liquid {

class Modem {
public:
    Modem(ModulationType type);
    ~Modem();

    void modulate(unsigned int symbol_in, std::complex<float>& symbol_out);
    void demodulate(std::complex<float> symbol_in, unsigned int& symbol_out);
    
    ModulationType type() const { return m_type; }
    unsigned int order() const { return m_order; }
    const std::complex<float>* constellation() const { return m_constellation.data(); }
    unsigned int constellation_size() const { return m_order; }

private:
    void init_constellation();
    
    ModulationType m_type;
    unsigned int m_order;
    std::vector<std::complex<float>> m_constellation;
};

} // namespace liquid
