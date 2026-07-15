## Example Summary

This example demonstrates a vector-based bearing fault detection application using an AI model on MSPM0G3507 microcontroller. The system contains vibration data from an ADXL355 accelerometer in flash. The appplication extracts feature vectors from this data using advanced signal processing, and performs inference using a pre-trained neural network model to classify bearing fault conditions. The detected fault class is indicated by toggling onboard LEDs.

In this example the inference happens using the M0+ CPU itself and no hardware accelerator is used.

The class to LED color mapping is shown below:
   - **LED1 (Blue)**: Class 0 (Normal)
   - **LED2 (Red)**: Class 1 (Contaminated)
   - **LED3 (Green)**: Class 2 (Erosion)
   - **LED1+LED2 (Purple)**: Class 3 (Flaking)
   - **LED2+LED3 (Yellow)**: Class 4 (No Lubrication)
   - **LED1+LED2+LED3 (White)**: Class 5 (Localized Fault)

## AI Model Information

| Property | Value |
| --- | --- |
| Model Architecture | CNN |
| Number of Parameters | 3,600 |
| Input Shape | (1, 384) |
| Output Classes | 6 (Bearing Fault Classification) |
| Quantization | INT8 |

In the PyTorch training framework, neural networks are trained with optimizations (for example, aggressive quantization) that target TI MCUs. After training, the neural networks are compiled by the [TI Neural Network Compiler](https://software-dl.ti.com/mctools/nnc/mcu/users_guide/index.html). Options passed to the compiler determine which of the following actions the generated inference library performs:
1. Hardware accelerated inference using TinyEngine™ NPU.
2. Software-only inference using the CPU on the MCU.

The output from the TI Neural Network Compiler is an artifacts directory that will contain: A header file (for example, tvmgen_default.h), and a library file (for example, model.a). This makes the output from the compiler easier to integrate with the project.

## AI Performance

| Metric | Value |
| --- | --- |
| Accuracy | ~99% |
| Flash Usage | 6.39 kB |
| RAM Usage | 16.56 kB |
| Inference Latency (CPU) | 229.01 ms |
*Note: Performance metrics measured on LP-MSPM0G3507.*

### Device Migration Recommendations

This project was developed for a superset device included in the MSPM0 LaunchPad. Please visit the [CCS User's Guide](https://software-dl.ti.com/msp430/esd/MSPM0-SDK/latest/docs/english/tools/ccs_ide_guide/doc_guide/doc_guide-srcs/ccs_ide_guide.html#sysconfig-project-migration) for information about migrating to other MSPM0 devices.

### Low-Power Recommendations

TI recommends to terminate unused pins by setting the corresponding functions to GPIO and configure the pins to output low or input with internal pullup/pulldown resistor.

SysConfig allows developers to easily configure unused pins by selecting **Board**→**Configure Unused Pins**.

For more information about jumper configuration to achieve low-power using the MSPM0 LaunchPad, please visit the [LP-MSPM0G3507 User's Guide](https://www.ti.com/lit/slau873).

## Hardware Requirements

1. LP-MSPM0G3507 LaunchPad

## Example Usage

1. **Hardware Setup**
   - Ensure LEDs are connected to the appropriate GPIO pins for fault indication

2. **Operation**
   - In bearing_fault_class_data.h, the macro of the class to be predicted is set as ENABLE and rest as DISABLE. Only one class macro can be enabled at a time.
      - Example:
         - #define CLASS_1     ENABLE
         - #define CLASS_2     DISABLE
         - #define CLASS_3     DISABLE
         - #define CLASS_4     DISABLE
         - #define CLASS_5     DISABLE
         - #define CLASS_6     DISABLE
   - On startup, the application initializes and loads sample bearing vibration data from flash memory
   - The vibration data is processed to extract feature vectors
   - Feature vectors are fed to the AI model for bearing fault classification inference
   - The output class determines which LED(s) to illuminate, indicating the detected bearing condition
   - The LED indication updates continuously as new classifications are made:
     - **LED1 (Blue)**: Class 1 (Normal)
     - **LED2 (Green)**: Class 2 (Contaminated)
     - **LED3 (Red)**: Class 3 (Erosion)
     - **LED1+LED2 (Purple)**: Class 4 (Flaking)
     - **LED2+LED3 (Yellow)**: Class 5 (No Lubrication)
     - **LED1+LED2+LED3 (White)**: Class 6 (Localized Fault)

3. **Running the Example**
   - Compile and load the application on your MSPM0G3507 device
   - The system will continuously process the bearing vibration data and indicate detected fault classes via LEDs

## Software Details

- **Data Source**: Pre-recorded bearing vibration sample data stored in flash memory
- **Feature Extraction**: Vector-based feature extraction from bearing vibration signals using ARM CMSIS-DSP algorithms
- **AI Inference**: Runs a pre-trained CNN/ML model using TVM runtime on the M0+ core for bearing fault classification
- **Classification**: Outputs one of 6 bearing fault classes:
  - Class 1: Normal bearing condition
  - Class 2: Contaminated bearing
  - Class 3: Bearing erosion
  - Class 4: Bearing flaking
  - Class 5: No lubrication
  - Class 6: Localized fault
- **LED Indication**: Classification result is indicated by toggling onboard LEDs based on detected fault class
- **AI model used**: Pre-trained model details can be found in TensorLab repository

## Notes

- This example uses sample bearing vibration data from flash memory and does not require a physical vibration sensor
- The sample data includes various bearing fault condition patterns for testing the classification model
- Future SDK releases will include versions that acquire real-time data from accelerometer or vibration sensors
- Adjust LED pin assignments and sensor configuration as needed for your hardware setup
- The feature extraction process is optimized using ARM CMSIS-DSP for efficient computation on the M0+ core

## References

- [ModelZoo Example](https://github.com/TexasInstruments/tinyml-tensorlab/tree/main/tinyml-modelzoo/examples)
- MSPM0G3507 Technical Reference Manual [Link](https://www.ti.com/product/MSPM0G3507)
- [TI Neural Network Compiler Guide](https://software-dl.ti.com/mctools/nnc/mcu/users_guide/)
- TI Model Training Guide: [tinyml-tensorlab](https://github.com/TexasInstruments/tinyml-tensorlab/tree/main)
- EdgeAI Software Guide:[Link](https://dev.ti.com/tirex/explore/node?node=A__AKCnvqDed-Plz2JO5Umb3Q__MSPM0-SDK__a3PaaoK__LATEST)
- Tensorlab User Guide [Link](https://software-dl.ti.com/C2000/esd/mcu_ai/01_03_00/user_guide/index.html)
