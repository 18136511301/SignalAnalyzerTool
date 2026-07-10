#pragma once
#include <vector>

namespace liquid {

enum WindowType {
    WINDOW_HAMMING = 0,
    WINDOW_HANNING = 1,
    WINDOW_BLACKMAN = 2,
    WINDOW_KAISER = 3
};

class FirDes {
public:
    static std::vector<float> design_kaiser_lowpass(
        unsigned int n, float fc, float As);
    
    static std::vector<float> design_hamming_lowpass(
        unsigned int n, float fc);

private:
    static float bessel_i0(float x);
};

} // namespace liquid
