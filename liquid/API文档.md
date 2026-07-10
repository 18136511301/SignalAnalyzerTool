# liquid-dsp C++ DLL API 文档

版本: 1.0.0 | 编译器: VS2017 + C++11

---

## 使用方法

1. 将 `liquid_dll.h`、`liquid_api.h`、`liquid/core/types.h` 复制到你的项目
2. 将 `liquid.dll` 放到程序运行目录
3. 将 `liquid.lib` 添加到链接器输入
4. 在代码中 `#include "liquid_dll.h"` 即可调用

---

## 数据类型

### ComplexFloat
```c
struct ComplexFloat {
    float real;  // 实部
    float imag;  // 虚部
};
```

### SignalInfo (信号识别结果)
```c
struct SignalInfo {
    float center_freq;      // 中心频率 (Hz)
    float symbol_rate;      // 符号率 (symbols/s)
    float bandwidth;        // 带宽 (Hz)
    float snr;              // 信噪比 (dB)
    int   modulation_type;  // 调制类型
    int   modulation_order; // 调制阶数
};
```

### ErrorCode (错误码)
| 值 | 常量 | 含义 |
|----|------|------|
| 0 | LIQUID_OK | 成功 |
| 1 | LIQUID_EINT | 内部错误 |
| 2 | LIQUID_EIOBJ | 无效对象 |
| 3 | LIQUID_EICONFIG | 无效配置 |
| 4 | LIQUID_EIVAL | 无效数值 |
| 8 | LIQUID_ENOINIT | 未初始化 |
| 12 | LIQUID_ENOIMP | 未实现 |

### ModulationType (调制类型)
| 值 | 常量 | 说明 |
|----|------|------|
| 0 | LIQUID_MODEM_BPSK | 二相相移键控 (1 bit/symbol) |
| 1 | LIQUID_MODEM_QPSK | 四相相移键控 (2 bits/symbol) |
| 2 | LIQUID_MODEM_8PSK | 八相相移键控 (3 bits/symbol) |
| 11 | LIQUID_MODEM_16QAM | 16QAM (4 bits/symbol) |
| 13 | LIQUID_MODEM_64QAM | 64QAM (6 bits/symbol) |
| 15 | LIQUID_MODEM_256QAM | 256QAM (8 bits/symbol) |
| 20 | LIQUID_MODEM_FSK | 频移键控 |
| 21 | LIQUID_MODEM_GMSK | 高斯最小频移键控 |

---

## FFT 模块 - 快速傅里叶变换

### liquid_fft_create
```c
void* liquid_fft_create(unsigned int nfft);
```
创建FFT对象。
- `nfft`: FFT点数，必须是2的幂 (64, 128, 256, 512, 1024...)
- 返回: FFT句柄，失败返回NULL

### liquid_fft_destroy
```c
void liquid_fft_destroy(void* q);
```
销毁FFT对象。

### liquid_fft_execute_forward
```c
int liquid_fft_execute_forward(void* q, ComplexFloat* x, ComplexFloat* y);
```
执行正向FFT。
- `q`: FFT句柄
- `x`: 输入复数数组 (长度=nfft)
- `y`: 输出复数数组 (长度=nfft)，可以与x相同(原地计算)
- 返回: 0=成功

### liquid_fft_execute_inverse
```c
int liquid_fft_execute_inverse(void* q, ComplexFloat* x, ComplexFloat* y);
```
执行逆向FFT。
- `q`: FFT句柄
- `x`: 输入复数数组
- `y`: 输出复数数组
- 返回: 0=成功

### 使用示例
```c
// 计算1024点FFT
void* fft = liquid_fft_create(1024);
ComplexFloat x[1024], y[1024];

// 填充数据
for (int i = 0; i < 1024; i++) {
    x[i].real = sin(2 * 3.14159 * i / 1024);
    x[i].imag = 0;
}

// 执行FFT
liquid_fft_execute_forward(fft, x, y);

// y 现在包含频域结果

liquid_fft_destroy(fft);
```

---

## FIR 滤波器模块 - 有限脉冲响应滤波器

### liquid_firfilt_create_kaiser
```c
void* liquid_firfilt_create_kaiser(unsigned int n, float fc, float As);
```
使用Kaiser窗设计低通FIR滤波器。
- `n`: 滤波器阶数 (抽头数)，越大过渡带越窄
- `fc`: 归一化截止频率 (0~0.5，0.5=奈奎斯特频率)
- `As`: 阻带衰减 (dB)，典型值40~80
- 返回: 滤波器句柄

### liquid_firfilt_create
```c
void* liquid_firfilt_create(const float* taps, unsigned int n);
```
使用自定义抽头创建FIR滤波器。
- `taps`: 滤波器抽头系数数组
- `n`: 抽头数量
- 返回: 滤波器句柄

### liquid_firfilt_destroy
```c
void liquid_firfilt_destroy(void* q);
```
销毁FIR滤波器。

### liquid_firfilt_execute_cf32
```c
int liquid_firfilt_execute_cf32(void* q, ComplexFloat* x, ComplexFloat* y);
```
复信号滤波 (逐样本处理)。
- `q`: 滤波器句柄
- `x`: 输入样本
- `y`: 输出样本
- 返回: 0=成功

### liquid_firfilt_execute_rf32
```c
int liquid_firfilt_execute_rf32(void* q, float x, float* y);
```
实信号滤波 (逐样本处理)。
- `q`: 滤波器句柄
- `x`: 输入样本
- `y`: 输出样本指针
- 返回: 0=成功

### liquid_firfilt_reset
```c
void liquid_firfilt_reset(void* q);
```
重置滤波器内部状态 (清空延迟线)。

### 使用示例
```c
// 设计低通滤波器：截止频率0.1，阻带衰减60dB
void* fir = liquid_firfilt_create_kaiser(64, 0.1f, 60.0f);

// 滤波处理
float input = 1.0f;
float output;
liquid_firfilt_execute_rf32(fir, input, &output);

liquid_firfilt_destroy(fir);
```

---

## NCO 模块 - 数控振荡器

### liquid_nco_create
```c
void* liquid_nco_create(int type);
```
创建NCO对象。
- `type`: 0=正弦波输出, 1=复指数输出
- 返回: NCO句柄

### liquid_nco_destroy
```c
void liquid_nco_destroy(void* q);
```
销毁NCO。

### liquid_nco_set_frequency
```c
void liquid_nco_set_frequency(void* q, float dtheta);
```
设置频率 (弧度/样本)。
- `dtheta = 2 * PI * f / fs`
- `f`: 频率 (Hz)
- `fs`: 采样率 (Hz)
- 示例: 1kHz信号，采样率1MHz → `dtheta = 2 * 3.14159 * 1000 / 1000000`

### liquid_nco_get_frequency
```c
float liquid_nco_get_frequency(void* q);
```
获取当前频率。

### liquid_nco_execute
```c
void liquid_nco_execute(void* q, ComplexFloat* y);
```
输出一个复指数样本并推进相位。
- `y`: 输出 = cos(θ) + j*sin(θ)

### liquid_nco_mix
```c
void liquid_nco_mix(void* q, ComplexFloat x, ComplexFloat* y);
```
混频：输入信号乘以NCO输出的共轭。
- `x`: 输入信号
- `y`: 输出 = x * conj(nco)

### 使用示例
```c
// 生成1kHz复正弦波，采样率1MHz
void* nco = liquid_nco_create(1);  // 复指数模式
liquid_nco_set_frequency(nco, 2 * 3.14159f * 1000.0f / 1000000.0f);

ComplexFloat y;
liquid_nco_execute(nco, &y);
// y.real = cos(θ), y.imag = sin(θ)

liquid_nco_destroy(nco);
```

---

## Modem 模块 - 调制解调器

### liquid_modem_create
```c
void* liquid_modem_create(int type);
```
创建调制解调器。
- `type`: 调制类型 (ModulationType枚举)
- 常用值: LIQUID_MODEM_BPSK(0), LIQUID_MODEM_QPSK(1), LIQUID_MODEM_16QAM(11)
- 返回: Modem句柄

### liquid_modem_destroy
```c
void liquid_modem_destroy(void* q);
```
销毁调制解调器。

### liquid_modem_modulate
```c
int liquid_modem_modulate(void* q, unsigned int symbol_in, ComplexFloat* symbol_out);
```
调制：将符号索引映射到星座点。
- `symbol_in`: 符号索引 (0 ~ order-1)
- `symbol_out`: 输出星座点
- 返回: 0=成功

### liquid_modem_demodulate
```c
int liquid_modem_demodulate(void* q, ComplexFloat symbol_in, unsigned int* symbol_out);
```
解调：将接收样本判决为最近的星座点。
- `symbol_in`: 接收的复数样本 (可能带噪声)
- `symbol_out`: 输出判决后的符号索引
- 返回: 0=成功

### liquid_modem_order
```c
unsigned int liquid_modem_order(void* q);
```
获取调制阶数 (QPSK=4, 16QAM=16)。

### 使用示例
```c
// QPSK调制解调
void* modem = liquid_modem_create(LIQUID_MODEM_QPSK);

// 调制符号0
ComplexFloat tx;
liquid_modem_modulate(modem, 0, &tx);
// tx ≈ (0.7071 + j*0.7071)

// 解调 (加噪后)
ComplexFloat rx = {0.6f, 0.8f};  // 带噪声的接收信号
unsigned int symbol;
liquid_modem_demodulate(modem, rx, &symbol);
// symbol = 0 (判决结果)

liquid_modem_destroy(modem);
```

---

## AGC 模块 - 自动增益控制

### liquid_agc_create
```c
void* liquid_agc_create(float bt, float signal_level);
```
创建AGC对象。
- `bt`: 环路带宽 (0~1)，越小越平滑，典型值0.01
- `signal_level`: 目标信号电平，通常设为1.0
- 返回: AGC句柄

### liquid_agc_destroy
```c
void liquid_agc_destroy(void* q);
```
销毁AGC。

### liquid_agc_execute
```c
void liquid_agc_execute(void* q, ComplexFloat* x);
```
执行AGC处理 (就地修改)。
- `x`: 输入/输出复数样本
- 处理后x的幅度被归一化到signal_level

### liquid_agc_get_rssi
```c
float liquid_agc_get_rssi(void* q);
```
获取当前接收信号强度 (RSSI)。

### liquid_agc_reset
```c
void liquid_agc_reset(void* q);
```
重置AGC内部状态。

### 使用示例
```c
void* agc = liquid_agc_create(0.01f, 1.0f);

ComplexFloat x = {100.0f, 50.0f};  // 大幅度信号
liquid_agc_execute(agc, &x);
// x 现在幅度约为 1.0

printf("RSSI: %.2f dB\n", 10*log10(liquid_agc_get_rssi(agc)));

liquid_agc_destroy(agc);
```

---

## Random 模块 - 随机数生成

### liquid_rand_create
```c
void* liquid_rand_create(unsigned int seed);
```
创建随机数生成器。
- `seed`: 随机种子，0=使用系统随机种子

### liquid_rand_destroy
```c
void liquid_rand_destroy(void* q);
```
销毁随机数生成器。

### liquid_randf
```c
float liquid_randf(void* q);
```
生成 [0, 1) 均匀分布随机浮点数。

### liquid_randn
```c
float liquid_randn(void* q);
```
生成均值0、方差1的高斯分布随机浮点数。

### 使用示例
```c
void* rng = liquid_rand_create(0);

for (int i = 0; i < 10; i++) {
    float uniform_val = liquid_randf(rng);  // [0, 1)
    float gaussian_val = liquid_randn(rng); // N(0, 1)
    printf("uniform: %.4f, gaussian: %.4f\n", uniform_val, gaussian_val);
}

liquid_rand_destroy(rng);
```

---

## 信号识别模块

### liquid_signal_identify_cf32
```c
int liquid_signal_identify_cf32(
    const ComplexFloat* iq_data, unsigned int num_samples,
    float sample_rate, SignalInfo* info);
```
从复信号IQ数据识别信号参数。
- `iq_data`: 输入IQ数据 (ComplexFloat数组)
- `num_samples`: 样本数
- `sample_rate`: 采样率 (Hz)
- `info`: 输出识别结果
- 返回: 0=成功
- 识别内容: 中心频率、符号率、带宽、信噪比、调制类型

### liquid_signal_identify_rf32
```c
int liquid_signal_identify_rf32(
    const float* iq_data, unsigned int num_samples,
    float sample_rate, SignalInfo* info);
```
从实信号识别参数。
- `iq_data`: 输入IQ数据 (float数组，I,Q,I,Q...交替存储)
- `num_samples`: 复数样本数 (不是float个数)
- `sample_rate`: 采样率 (Hz)
- `info`: 输出识别结果
- 返回: 0=成功

### 使用示例
```c
// 识别信号
float iq_data[2048];  // 你的IQ数据
SignalInfo info;

int ret = liquid_signal_identify_rf32(iq_data, 1024, 2400000.0f, &info);
if (ret == 0) {
    printf("中心频率: %.0f Hz\n", info.center_freq);
    printf("符号率: %.0f symbols/s\n", info.symbol_rate);
    printf("带宽: %.0f Hz\n", info.bandwidth);
    printf("信噪比: %.1f dB\n", info.snr);
    printf("调制类型: %d\n", info.modulation_type);
}
```

---

## 解调模块

### liquid_demod_create
```c
void* liquid_demod_create(int modulation_type, float symbol_rate, float sample_rate);
```
创建解调器。
- `modulation_type`: 调制类型 (ModulationType枚举)
- `symbol_rate`: 符号率 (symbols/s)
- `sample_rate`: 采样率 (samples/s)
- 返回: 解调器句柄

### liquid_demod_destroy
```c
void liquid_demod_destroy(void* q);
```
销毁解调器。

### liquid_demod_execute
```c
int liquid_demod_execute(void* q, const ComplexFloat* iq_data,
                         unsigned int n, unsigned int* symbols, unsigned int* num_symbols);
```
执行解调。
- `iq_data`: 输入IQ数据
- `n`: 输入样本数
- `symbols`: 输出符号数组 (调用者分配)
- `num_symbols`: 输出实际解调的符号数
- 返回: 0=成功

### 使用示例
```c
// 完整的信号识别+解调流程
float iq_data[4096];  // 你的IQ数据
SignalInfo info;

// 第一步：识别信号
liquid_signal_identify_rf32(iq_data, 2048, 2400000.0f, &info);

// 第二步：根据识别结果解调
void* demod = liquid_demod_create(
    info.modulation_type,
    info.symbol_rate,
    2400000.0f
);

unsigned int symbols[4096];
unsigned int num_symbols;
liquid_demod_execute(demod, (ComplexFloat*)iq_data, 2048, symbols, &num_symbols);

// 输出解调结果
for (unsigned int i = 0; i < num_symbols; i++) {
    printf("symbol[%u] = %u\n", i, symbols[i]);
}

liquid_demod_destroy(demod);
```

---

## 工具函数

### liquid_version
```c
const char* liquid_version(void);
```
获取库版本字符串。

### liquid_error_string
```c
const char* liquid_error_string(int code);
```
将错误码转换为可读字符串。

---

## 完整工作流示例

```c
#include "liquid_dll.h"
#include <stdio.h>

int main() {
    // 假设已有IQ数据
    float iq_data[4096];
    // ... 填充你的IQ数据 ...

    // 1. 识别信号
    SignalInfo info;
    int ret = liquid_signal_identify_rf32(iq_data, 2048, 2400000.0f, &info);
    if (ret != 0) {
        printf("信号识别失败: %s\n", liquid_error_string(ret));
        return 1;
    }

    printf("=== 信号识别结果 ===\n");
    printf("中心频率: %.0f Hz\n", info.center_freq);
    printf("符号率: %.0f symbols/s\n", info.symbol_rate);
    printf("带宽: %.0f Hz\n", info.bandwidth);
    printf("信噪比: %.1f dB\n", info.snr);
    printf("调制类型: %d\n", info.modulation_type);

    // 2. 解调
    void* demod = liquid_demod_create(
        info.modulation_type,
        info.symbol_rate,
        2400000.0f
    );

    unsigned int symbols[4096];
    unsigned int num_symbols;
    ret = liquid_demod_execute(demod, (ComplexFloat*)iq_data, 2048, symbols, &num_symbols);

    if (ret == 0) {
        printf("\n=== 解调结果 ===\n");
        printf("解调符号数: %u\n", num_symbols);
        for (unsigned int i = 0; i < num_symbols && i < 20; i++) {
            printf("symbol[%u] = %u\n", i, symbols[i]);
        }
    }

    liquid_demod_destroy(demod);
    return 0;
}
```
