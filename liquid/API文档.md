# liquid-dsp C++ DLL API 文档 (V2)

版本: 2.0.0 | 编译器: VS2017 + C++11

---

## 数据类型

### ComplexFloat
```c
struct ComplexFloat {
    float real;  // 实部
    float imag;  // 虚部
};
```

### SignalType (信号类型)
```c
enum SignalType {
    LIQUID_SIGNAL_REAL = 0,     // 实信号（单通道 Mono）
    LIQUID_SIGNAL_COMPLEX = 1   // 复信号（IQ 交织）
};
```

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

## V2 API（推荐使用）

### 信号识别 V2

```c
int liquid_signal_identify_v2(
    const IdentifyParams* params,
    IdentifyResult* result);
```

**输入参数 IdentifyParams：**
| 字段 | 类型 | 说明 |
|------|------|------|
| `data` | `const void*` | 原始二进制数据（直接从文件读取） |
| `data_len` | `unsigned int` | 数据长度（**字节数**） |
| `bit_width` | `int` | 采样位宽：16（int16_t）或 32（float） |
| `signal_type` | `int` | 信号类型：0=实信号，1=复信号 |
| `sample_rate` | `float` | 采样率（Hz） |

**输出参数 IdentifyResult：**
| 字段 | 类型 | 说明 |
|------|------|------|
| `center_freq` | `float` | 估计的中心/载波频率（Hz） |
| `symbol_rate` | `float` | 估计的符号率（symbols/s） |
| `bandwidth` | `float` | 3dB 带宽（Hz） |
| `snr` | `float` | 信噪比（dB） |
| `modulation_type` | `int` | 调制类型（ModulationType 枚举值） |
| `modulation_order` | `int` | 星座图阶数（2, 4, 8, 16...） |
| `bits_per_symbol` | `int` | 每符号比特数（1, 2, 3, 4...） |
| `confidence` | `int` | 分类置信度（0-100） |

**返回值：** 0=成功，其他=失败

**使用示例：**
```c
// 从文件加载 16bit 实信号数据
FILE* f = fopen("bpsk_10k_100k.dat", "rb");
fseek(f, 0, SEEK_END);
unsigned int file_size = ftell(f);
fseek(f, 0, SEEK_SET);
void* buf = malloc(file_size);
fread(buf, 1, file_size, f);
fclose(f);

// 识别
IdentifyParams params;
params.data = buf;
params.data_len = file_size;
params.bit_width = 16;              // 16bit 采样
params.signal_type = LIQUID_SIGNAL_REAL;  // 实信号
params.sample_rate = 100000.0f;     // 100kHz 采样率

IdentifyResult result;
int ret = liquid_signal_identify_v2(&params, &result);

if (ret == 0) {
    printf("中心频率: %.1f Hz\n", result.center_freq);
    printf("符号率: %.1f sym/s\n", result.symbol_rate);
    printf("3dB带宽: %.1f Hz\n", result.bandwidth);
    printf("信噪比: %.1f dB\n", result.snr);
    printf("调制类型: %d (阶数: %d, %d bit/symbol)\n",
           result.modulation_type, result.modulation_order, result.bits_per_symbol);
    printf("置信度: %d%%\n", result.confidence);
}

free(buf);
```

---

### 信号解调 V2

```c
int liquid_demodulate_v2(
    const DemodInput* input,
    DemodOutput* output);
```

**输入参数 DemodInput：**
| 字段 | 类型 | 说明 |
|------|------|------|
| `data` | `const void*` | 原始二进制数据 |
| `data_len` | `unsigned int` | 数据长度（字节数） |
| `bit_width` | `int` | 采样位宽：16 或 32 |
| `signal_type` | `int` | 信号类型：0=实信号，1=复信号 |
| `sample_rate` | `float` | 采样率（Hz） |
| `carrier_freq` | `float` | 载波频率（Hz），通常来自识别结果 |
| `symbol_rate` | `float` | 调制速率（symbols/s），通常来自识别结果 |
| `modulation_type` | `int` | 调制方式（ModulationType 枚举值） |

**输出参数 DemodOutput：**
| 字段 | 类型 | 说明 |
|------|------|------|
| `hard_symbols` | `unsigned int*` | 硬判输出（符号索引），调用者分配 |
| `soft_symbols` | `ComplexFloat*` | 软判输出（判决前 I/Q），调用者分配 |
| `hard_count` | `unsigned int` | 实际写入的硬判符号数 |
| `soft_count` | `unsigned int` | 实际写入的软判符号数 |
| `hard_capacity` | `unsigned int` | 硬判缓冲区容量 |
| `soft_capacity` | `unsigned int` | 软判缓冲区容量 |
| `locked` | `bool` | PLL 锁定指示 |
| `freq_offset` | `float` | 估计的残余频偏（Hz） |

**返回值：** 0=成功，其他=失败

**使用示例：**
```c
// 假设已有 IdentifyResult result 和文件数据 buf/file_size

// 准备输入
DemodInput demodIn;
demodIn.data = buf;
demodIn.data_len = file_size;
demodIn.bit_width = 16;
demodIn.signal_type = LIQUID_SIGNAL_REAL;
demodIn.sample_rate = 100000.0f;
demodIn.carrier_freq = result.center_freq;  // 来自识别
demodIn.symbol_rate = result.symbol_rate;    // 来自识别
demodIn.modulation_type = result.modulation_type;

// 估算符号数，分配缓冲区
unsigned int estSymbols = 100000;
unsigned int* hardBuf = malloc(estSymbols * sizeof(unsigned int));
ComplexFloat* softBuf = malloc(estSymbols * sizeof(ComplexFloat));

// 准备输出
DemodOutput demodOut;
demodOut.hard_symbols = hardBuf;
demodOut.soft_symbols = softBuf;
demodOut.hard_capacity = estSymbols;
demodOut.soft_capacity = estSymbols;

int ret = liquid_demodulate_v2(&demodIn, &demodOut);

if (ret == 0) {
    printf("解调 %u 个符号\n", demodOut.hard_count);
    printf("PLL锁定: %s\n", demodOut.locked ? "是" : "否");
    printf("残余频偏: %.1f Hz\n", demodOut.freq_offset);

    // 硬判结果：0 或 1（BPSK）
    for (unsigned int i = 0; i < demodOut.hard_count && i < 20; i++) {
        printf("symbol[%u] = %u\n", i, hardBuf[i]);
    }

    // 软判结果：判决前的 I/Q 值（用于星座图）
    for (unsigned int i = 0; i < demodOut.soft_count && i < 20; i++) {
        printf("soft[%u] = (%.3f, %.3f)\n", i, softBuf[i].real, softBuf[i].imag);
    }
}

free(hardBuf);
free(softBuf);
```

---

## 完整工作流示例

```c
#include "liquid/liquid_dll.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    // 1. 读取数据文件
    FILE* f = fopen("bpsk_prbs11_10k_100k.dat", "rb");
    if (!f) { printf("Cannot open file\n"); return 1; }
    fseek(f, 0, SEEK_END);
    unsigned int file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    void* buf = malloc(file_size);
    fread(buf, 1, file_size, f);
    fclose(f);

    // 2. 信号识别
    IdentifyParams idParams;
    idParams.data = buf;
    idParams.data_len = file_size;
    idParams.bit_width = 16;
    idParams.signal_type = LIQUID_SIGNAL_REAL;
    idParams.sample_rate = 100000.0f;

    IdentifyResult idResult;
    int ret = liquid_signal_identify_v2(&idParams, &idResult);
    if (ret != 0) {
        printf("识别失败\n");
        free(buf);
        return 1;
    }

    printf("=== 信号识别结果 ===\n");
    printf("中心频率: %.1f Hz\n", idResult.center_freq);
    printf("符号率: %.1f sym/s\n", idResult.symbol_rate);
    printf("3dB带宽: %.1f Hz\n", idResult.bandwidth);
    printf("信噪比: %.1f dB\n", idResult.snr);
    printf("调制类型: %d (%d阶, %d bit/symbol)\n",
           idResult.modulation_type, idResult.modulation_order, idResult.bits_per_symbol);
    printf("置信度: %d%%\n", idResult.confidence);

    // 3. 解调
    unsigned int estSym = file_size / 2 / 10 + 1024;  // rough estimate
    unsigned int* hardBuf = (unsigned int*)malloc(estSym * sizeof(unsigned int));
    ComplexFloat* softBuf = (ComplexFloat*)malloc(estSym * sizeof(ComplexFloat));

    DemodInput demodIn;
    demodIn.data = buf;
    demodIn.data_len = file_size;
    demodIn.bit_width = 16;
    demodIn.signal_type = LIQUID_SIGNAL_REAL;
    demodIn.sample_rate = 100000.0f;
    demodIn.carrier_freq = idResult.center_freq;
    demodIn.symbol_rate = idResult.symbol_rate;
    demodIn.modulation_type = idResult.modulation_type;

    DemodOutput demodOut;
    demodOut.hard_symbols = hardBuf;
    demodOut.soft_symbols = softBuf;
    demodOut.hard_capacity = estSym;
    demodOut.soft_capacity = estSym;

    ret = liquid_demodulate_v2(&demodIn, &demodOut);
    if (ret == 0) {
        printf("\n=== 解调结果 ===\n");
        printf("解调符号数: %u\n", demodOut.hard_count);
        printf("PLL锁定: %s\n", demodOut.locked ? "是" : "否");
        printf("残余频偏: %.1f Hz\n", demodOut.freq_offset);

        // 输出前 20 个硬判符号
        for (unsigned int i = 0; i < demodOut.hard_count && i < 20; i++) {
            printf("  hard[%u] = %u\n", i, hardBuf[i]);
        }
    }

    free(hardBuf);
    free(softBuf);
    free(buf);
    return 0;
}
```

---

## Legacy API（向后兼容）

### liquid_signal_identify_cf32
```c
int liquid_signal_identify_cf32(
    const ComplexFloat* iq_data, unsigned int num_samples,
    float sample_rate, SignalInfo* info);
```
从复信号 IQ 数据识别信号参数。

### liquid_signal_identify_rf32
```c
int liquid_signal_identify_rf32(
    const float* iq_data, unsigned int num_samples,
    float sample_rate, SignalInfo* info);
```
从实信号识别参数。`num_samples` 为 float 个数（实信号样本数）。

### liquid_demod_create / liquid_demod_execute / liquid_demod_destroy
Legacy 解调接口，逐块处理。建议使用 V2 API 替代。

---

## 工具函数

| 函数 | 说明 |
|------|------|
| `liquid_version()` | 获取库版本字符串 |
| `liquid_error_string(code)` | 错误码转可读字符串 |
| `liquid_fft_create/destroy/execute` | FFT 运算 |
| `liquid_firfilt_create/destroy/execute` | FIR 滤波器 |
| `liquid_nco_create/destroy/execute` | 数控振荡器 |
| `liquid_modem_create/destroy/modulate/demodulate` | 调制解调器 |
| `liquid_agc_create/destroy/execute` | 自动增益控制 |
| `liquid_rand_create/destroy/randf/randn` | 随机数生成 |
