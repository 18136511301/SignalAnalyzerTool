#include "firdes.h"
#include <cmath>
#include <algorithm>

namespace liquid {

float FirDes::bessel_i0(float x) {
    float sum = 1.0f;
    float term = 1.0f;
    for (int k = 1; k < 25; k++) {
        term *= (x / 2.0f) * (x / 2.0f) / (static_cast<float>(k) * static_cast<float>(k));
        sum += term;
    }
    return sum;
}

std::vector<float> FirDes::design_kaiser_lowpass(unsigned int n, float fc, float As) {
    std::vector<float> h(n);
    float beta = 0.1102f * (As - 8.7f);
    float m = static_cast<float>(n - 1) / 2.0f;
    float i0_beta = bessel_i0(beta);
    
    for (unsigned int i = 0; i < n; i++) {
        float x = static_cast<float>(i) - m;
        float ideal = 2.0f * fc * std::sin(3.14159265358979323846f * x) / (3.14159265358979323846f * x);
        if (std::fabs(x) < 1e-6f) ideal = 2.0f * fc;
        
        float win = bessel_i0(beta * std::sqrt(1.0f - (2.0f * x / (n - 1)) * (2.0f * x / (n - 1)))) / i0_beta;
        h[i] = ideal * win;
    }
    
    float sum = 0;
    for (unsigned int i = 0; i < n; i++) sum += h[i];
    for (unsigned int i = 0; i < n; i++) h[i] /= sum;
    
    return h;
}

std::vector<float> FirDes::design_hamming_lowpass(unsigned int n, float fc) {
    std::vector<float> h(n);
    float m = static_cast<float>(n - 1) / 2.0f;
    
    for (unsigned int i = 0; i < n; i++) {
        float x = static_cast<float>(i) - m;
        float ideal = 2.0f * fc * std::sin(3.14159265358979323846f * x) / (3.14159265358979323846f * x);
        if (std::fabs(x) < 1e-6f) ideal = 2.0f * fc;
        
        float win = 0.54f - 0.46f * std::cos(2.0f * 3.14159265358979323846f * static_cast<float>(i) / (n - 1));
        h[i] = ideal * win;
    }
    
    float sum = 0;
    for (unsigned int i = 0; i < n; i++) sum += h[i];
    for (unsigned int i = 0; i < n; i++) h[i] /= sum;
    
    return h;
}

} // namespace liquid
