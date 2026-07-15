# Wakeword Detection using NPU AI on LP-MSPM0G5187

A real-time wakeword (keyword) detection application running on Texas Instruments' MSPM0G5187 microcontroller, leveraging the on-chip Neural Processing Unit (NPU) for efficient AI inference. The system continuously listens for the wakeword **"OK Kilby"** using audio data captured via the ADC peripheral, extracts features, and runs a quantized neural network to detect the keyword in real time.

---

## Table of Contents

- [Overview](#overview)
- [Hardware Requirements](#hardware-requirements)
- [Project Structure](#project-structure)
- [System Architecture](#system-architecture)
- [Audio Pipeline](#audio-pipeline)
- [Feature Extraction](#feature-extraction)
- [Neural Network Model](#neural-network-model)
- [Post-Inference Processing](#post-inference-processing)
- [Configuration](#configuration)
- [Build Instructions](#build-instructions)
- [Pin Mapping](#pin-mapping)
- [Memory Layout](#memory-layout)
- [Power Strategy](#power-strategy)
- [References](#references)

---

## Overview

This project implements an always-on wakeword detection system that:

1. Captures audio at **8 kHz** from an analog microphone connected to the ADC12 peripheral
2. Extracts **LPC (Linear Predictive Coding)** features from each 20 ms audio frame
3. Maintains a **2-second sliding window** of features as input to the neural network
4. Runs a quantized **TCDResNet** ONNX model via TVM on the hardware **NPU accelerator**
5. Lights an **LED** when 6 consecutive positive predictions confirm wakeword detection

> **Note on `data_collection.c`:**  this file does **not** use SPI to read audio data.
> It initializes system peripherals and configures the **ADC12** to capture audio samples from the
> analog microphone. 
---

## Hardware Requirements

| Component    | Details                                          |
|--------------|--------------------------------------------------|
| MCU          | MSPM0G5187 (ARM Cortex-M0+, integrated NPU)     |
| LaunchPad    | LP-MSPM0G5187                                    |
| Microphone   | Analog microphone connected to PB22 (AIN24)      |
| IDE          | Code Composer Studio (CCS)                       |
| Compiler     | TICLANG 2.1.3 LTS (TI LLVM)                     |
| SDK          | MSPM0 SDK with Edge AI / TinyEngine support      |

---

## Project Structure

```
wakeword_detection_npu_ai_LP_MSPM0G5187_nortos_ticlang/
│
├── main.c                           # Application entry point and main loop
├── common.h                         # Shared constants, buffer sizes, extern declarations
│
├── data_collection.c / data_collection.h            # Peripheral init and ADC ISR for audio capture
│                                    # (Note: reads audio from ADC, NOT SPI)
├── feature_extraction.c / .h        # LPC feature extraction
├── buffer_manager.c / .h            # Sliding window buffer management
├── post_inference.c / .h            # Wakeword detection decision logic and LED control
│
├── lpc_float/                       # LPC algorithm implementation
│   ├── lpc.c                        # LPC computation (autocorrelation, Levinson-Durbin)
│   ├── lpc_defs.h                   # LPC parameters (frame length, order, frequency bins)
│   ├── lpc_constants.h              # Precomputed LPC constants
│   └── lpc_input_output.h           # Test vectors for LPC validation
│
├── model/                           # TVM compiled neural network
│   ├── model.a                      # Compiled model binary (~75 KB)
│   └── tvmgen_default.h             # TVM model C API (inputs, outputs, run function)
│
├── wakeword_detection_npu_ai.syscfg # SysConfig peripheral configuration
├── targetConfigs/
│   └── MSPM0G5187.ccxml             # CCS debug/flash target configuration
│
├── Debug/                           # Auto-generated build artifacts
│   ├── *.o / *.d                    # Object and dependency files
│   ├── *.out / *.map                # Executable and linker map
│   └── ti_msp_dl_config.*           # SysConfig-generated peripheral init code
│
├── .project / .cproject             # CCS project descriptors
└── .settings/                       # IDE workspace settings
```

---

## System Architecture

```
┌──────────────────────────────────────────────────────────────────┐
│                    LP-MSPM0G5187 LaunchPad                       │
│                                                                  │
│  ┌────────────┐  8 kHz ISR   ┌───────────────────────────────┐  │
│  │ Analog Mic │─────────────▶│  ADC12 (PB22 / AIN24)         │  │
│  └────────────┘              │  12/14-bit, HW averaging x8   │  │
│                              └───────────────┬───────────────┘  │
│                                              │ shift_audio_buffer│
│                              ┌───────────────▼───────────────┐  │
│                              │  Raw Audio Buffer [160]        │  │
│                              │  (normalize + circular fill)   │  │
│                              └───────────────┬───────────────┘  │
│                                              │ every 20 ms       │
│                              ┌───────────────▼───────────────┐  │
│                              │  Feature Extraction            │  │
│                              │  LPC  → 70 coefficients/frame  │  │
│                              │                                │  │
│                              └───────────────┬───────────────┘  │
│                                              │ shift_NN_input    │
│                              ┌───────────────▼───────────────┐  │
│                              │  Sliding Feature Window        │  │
│                              │  100 frames × 70 features      │  │
│                              │  ≈ 2 seconds of audio context  │  │
│                              └───────────────┬───────────────┘  │
│                                              │ tvmgen_default_run│
│                              ┌───────────────▼───────────────┐  │
│                              │  NPU Inference (TVM)           │  │
│                              │  TCDResNet INT8 ONNX model     │  │
│                              │  Output: [class0, class1]      │  │
│                              └───────────────┬───────────────┘  │
│                                              │ post_inference_ww │
│                              ┌───────────────▼───────────────┐  │
│                              │  Detection Logic               │  │
│                              │  6× consecutive → Blue LED ON  │  │
│                              └───────────────────────────────┘  │
└──────────────────────────────────────────────────────────────────┘
```

---

## Audio Pipeline

### 1. ADC Sampling — `data_collection.c`

`data_collection.c` It handles:

- System clock configuration (SYSPLL boosted to ~48 MHz)
- Full peripheral initialization via `SYSCFG_DL_init()`
- ADC12 interrupt enable
- The `ADC12_0_INST_IRQHandler()` ISR, which fires at **8 kHz**

**ADC sampling flow:**

```
TIMG0 (Timer_0) fires every 125 µs
  → triggers ADC12_0 conversion on AIN24 (PB22)
    → MEM0_RESULT_LOADED interrupt fires
      → read 12/14-bit ADC result
        → shift_audio_buffer(sample)
          → samples_count++
```

### 2. Sample Normalization — `buffer_manager.c`

Each raw ADC sample is centered and scaled before buffering:

```c
normalized = (raw_sample - NORMALIZE_VALUE) << SHIFT_BITS;
```

| ADC Mode | NORMALIZE_VALUE | SHIFT_BITS |
|----------|-----------------|------------|
| 14-bit   | 8192            | 2          |
| 12-bit   | 2048            | 4          |

Once 160 normalized samples are collected (20 ms of audio), the buffer is copied for feature extraction and `samples_count` resets.

---

## Feature Extraction

Feature extraction mode is selected at **compile time** via preprocessor macros.

### LPC Mode (default, recommended)

**Linear Predictive Coding** — computes a 70-element magnitude spectrum from each 20 ms audio frame.

| Parameter         | Value                        |
|-------------------|------------------------------|
| Samples per frame | 160 (20 ms @ 8 kHz)          |
| Window length     | 240 samples (30 ms, 10 ms overlap) |
| LPC order         | 10                           |
| Output features   | 70 coefficients per frame    |
| Quantization      | int16 → int8                 |

**Processing steps (`feature_extraction.c`):**
1. `get_lpc()` applies the Levinson-Durbin algorithm on a 240-sample windowed frame
2. Computes autocorrelation and derives 70 frequency-domain LPC coefficients
3. Quantizes coefficients to int8:
   ```
   int8 = clamp((coeff * 157 + 1*256) >> 13, -128, 127)
   ```
4. Appends new feature vector to the sliding window via `shift_NN_input_buffer()`


### Sliding Window — `buffer_manager.c`

LPC maintain a rolling history of extracted features as input to the neural network:

| Mode | Window shape                   | Total size  |
|------|-------------------------------|-------------|
| LPC  | 100 frames × 70 features (int8) | 7,000 bytes |

`shift_NN_input_buffer()` shifts the window left by one row (discarding the oldest frame) and appends the newest feature vector at the end, implementing an efficient sliding window with `memmove`/`memcpy`.

---

## Neural Network Model

### Model Details

| Property        | Value                                         |
|-----------------|-----------------------------------------------|
| Source model    | `ok_kilby_lpc_tcdresnet_int_model.onnx`       |
| Architecture    | TCDResNet (Temporal Conv + Depthwise Residual) |
| Wakeword        | "OK Kilby"                                    |
| Compiler        | TI MCU Neural Network Compiler 2.1.0          |
| Runtime         | TVM on TI NPU hardware accelerator            |
| Input tensor    | `(1, 1, 100, 70)` int8                        |
| Output tensor   | `(1, 2)` int8                                 |
| Model binary    | `model/model.a` (~75 KB)                      |

### Input Quantization

Float feature values are quantized to int8 before being passed to the model:

```
input_int8 = clip(((int32)(input_float + 2.5188484) * 6) >> 0, -128, 127)
```

### TVM C API (`model/tvmgen_default.h`)

```c
#include "model/tvmgen_default.h"

// Point inputs to the sliding feature window
struct tvmgen_default_inputs inputs = {
    .onnx__Add_0 = audio_features   // int8*, shape (1,1,100,70)
};

// Point outputs to result buffer
struct tvmgen_default_outputs outputs = {
    .output = output                 // int8*, shape (1,2)
};

// Run inference on NPU (non-blocking, completion flagged via interrupt)
tvmgen_default_run(&inputs, &outputs);
// Waits on tvmgen_default_finished flag set by NPU interrupt
```

---

## Post-Inference Processing

`post_inference_ww()` in `post_inference.c` applies a **debounced threshold** decision on the two output class scores:

```
Class 0 = wakeword ("OK Kilby")
Class 1 = background / other

if output[0] > output[1]:
    wwDetectCounter++
    if wwDetectCounter >= 6:
        → Turn ON blue LED (PB13)    ← Confirmed wakeword detection
else:
    wwDetectCounter = 0
    → Turn OFF blue LED (PB13)
```

- **6 consecutive positive frames** (~120 ms) are required before a detection is confirmed
- This debouncing prevents false triggers from momentary noise
- The LED stays ON as long as positive predictions continue

---

## Configuration

### Compile-Time Flags

| Flag               | Description                                              |
|--------------------|----------------------------------------------------------|
| `LPC=1`            | Enable LPC feature extraction (default, recommended)     |
| `ADC_14B=1`        | Use 14-bit ADC precision (default)                       |
| `ADC_12B=1`        | Use 12-bit ADC precision                                 |
| `PROFILE_EXAMPLE=1`| Enable NPU cycle count profiling                         |

### Key Audio Parameters (`common.h`)

| Parameter              | LPC  | Default (FFT) |
|------------------------|------|---------------|
| `NUM_AUDIO_SAMPLES`    | 160  | 128           |
| `AUDIO_FEATURE_WINDOW` | 100  | 99            |
| `Feature output size`    | 70   | 16            |

### Clock Configuration

| Setting         | Value                          |
|-----------------|--------------------------------|
| PLL source      | External XTAL                  |
| SYSPLL QDIV     | ×5                             |
| UDIV            | ÷2                             |
| MCLK            | ~40–48 MHz                     |
| Timer_0 source  | LFCLK                          |
| ADC trigger rate| 8 kHz (every 125 µs)           |

---

## Build Instructions

1. **Install** Code Composer Studio (CCS) and the MSPM0 SDK.

2. **Import** the project into CCS:
   - File → Import → Code Composer Studio → Existing CCS Eclipse Projects
   - Browse to this directory and select it.

3. **Configure peripherals** (if needed) by opening `wakeword_detection_npu_ai.syscfg` in the SysConfig GUI and regenerating.

4. **Select feature extraction mode** in Project Properties → Build → Compiler → Predefined Symbols:
   - Add `LPC=1` for LPC features (recommended)

5. **Build** the project (Ctrl+B). The output is:
   ```
   Debug/wakeword_detection_npu_ai_LP_MSPM0G5187_nortos_ticlang.out
   ```

6. **Flash and debug** using the onboard XDS110 debugger via CCS → Run → Debug.

---

## Pin Mapping

| Signal             | MCU Pin      | Function                              |
|--------------------|--------------|---------------------------------------|
| Analog Mic Input   | PB22 (AIN24) | ADC12 channel 24 — audio input        |
| Blue LED           | PB13         | Wakeword detection indicator (LED_1)  |
| Secondary LED      | PA0          | General-purpose LED (LED_2)           |
| Debug GPIO         | PB21         | Test/sync output (GPIO_GRP_0)         |

---

## Memory Layout

| Region       | Approximate Size | Contents                                    |
|--------------|-----------------|----------------------------------------------|
| Flash        | ~75 KB          | `model.a` — compiled NPU model weights/kernels |
| Flash        | ~10 KB          | Application code                             |
| SRAM         | 7,000 bytes     | `audio_features[]` — LPC sliding feature window |
| SRAM         | 320 bytes       | `raw_audio_samples[]` + working copy buffer  |
| SRAM         | ~2 KB           | Stack, local variables, output buffers       |

---

## Power Strategy

The application uses **STANDBY0** power policy to minimize energy consumption between events:

- CPU sleeps between ADC interrupts (wakes every 125 µs)
- NPU is powered on only during inference (~5–10 ms per 20 ms frame)
- ADC is configured in manual power-down mode when idle
- Clock gating is applied to unused peripherals via SysConfig

---

## References

- [MSPM0G5187 Product Page](https://www.ti.com/product/MSPM0G5187)
- [MSPM0 SDK](https://www.ti.com/tool/MSPM0-SDK)
- [TI MCU Neural Network Compiler (NNC)](https://www.ti.com/tool/TI-MCU-NNC)
- [tinyml-tensorlab — TI Model Training](https://github.com/TexasInstruments/tinyml-tensorlab)
- [TI EdgeAI Software](https://www.ti.com/tool/TI-EDGE-AI-DEVKIT)
- [Apache TVM](https://tvm.apache.org/)
- MSPM0G5187 Technical Reference Manual (SLAU906)
