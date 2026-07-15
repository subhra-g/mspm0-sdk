## Example Summary

This example demonstrates a human fall detection classification application for a belt‑mounted safety system using an AI model on MSPM0G5187 microcontroller. The system collects vibration data from an BMI270 IMU present on TIDA-010997 EdgeAI BoosterPack, processes the signal, and performs inference using a pre-trained neural network model. The detected class is indicated by toggling onboard LEDs.

The dataset contains raw sensor data from an accelerometer worn by subjects performing various physical activities, sampled at 200 Hz with three input features: X-axis, Y-axis, and Z-axis acceleration. The data has been pre-processed into ~4.5k labeled CSV files, representing 19 Activities of Daily Living (ADLs) such as walking, jogging, sitting, and stair climbing, and 15 fall types, recorded across 38 subjects (23 young adults and 15 elderly) with multiple trials per activity.

In this example the inference happens using the TinyEngine™ NPU hardware accelerator, thus making it faster in comparison to using M0+ CPU to do the same.

The class to LED color mapping is shown below:
   - **Green**: Class 0 (ADL)
   - **Red**: Class 1 (Fall)

## AI Model Information

| Property | Value |
| --- | --- |
| Model Architecture | CNN |
| Number of Parameters | 6K |
| Input Shape | (1, 3, 64, 1) |
| Output Classes | 2 |
| Quantization | INT8 |

In the PyTorch training framework, neural networks are trained with optimizations (for example, aggressive quantization) that target TI MCUs. After training, the neural networks are compiled by the [TI Neural Network Compiler](https://software-dl.ti.com/mctools/nnc/mcu/users_guide/index.html). Options passed to the compiler determine which of the following actions the generated inference library performs:
1. Hardware accelerated inference using TinyEngine™ NPU.
2. Software-only inference using the CPU on the MCU.

The output from the TI Neural Network Compiler is an artifacts directory that will contain: A header file (for example, tvmgen_default.h), and a library file (for example, model.a). This makes the output from the compiler easier to integrate with the project.

## AI Performance

| Metric | Value |
| --- | --- |
| Accuracy | 97.65% |
| Flash Usage | 14 kB |
| RAM Usage | 0.7 kB |
| Inference Latency (NPU) | 0.67 ms |

*Note: Performance metrics measured on LP-MSPM0G5187 at 80 MHz with TinyEngine NPU.*

### Device Migration Recommendations

This project was developed for a superset device included in the MSPM0 LaunchPad. Please visit the [CCS User's Guide](https://software-dl.ti.com/msp430/esd/MSPM0-SDK/latest/docs/english/tools/ccs_ide_guide/doc_guide/doc_guide-srcs/ccs_ide_guide.html#sysconfig-project-migration) for information about migrating to other MSPM0 devices.

### Low-Power Recommendations

TI recommends to terminate unused pins by setting the corresponding functions to GPIO and configure the pins to output low or input with internal pullup/pulldown resistor.

SysConfig allows developers to easily configure unused pins by selecting **Board**→**Configure Unused Pins**.

For more information about jumper configuration to achieve low-power using the MSPM0 LaunchPad, please visit the [<LAUNCHPAD> User's Guide](https://www.ti.com/lit/<USER_GUIDE_LIT_NUMBER>).

## Hardware Requirements

1. LP-MSPM0G5187 microcontroller / LaunchPad
2. TIDA-010997 (EdgeAI BoosterPack)

## Example Usage

1. **Hardware Setup**
   - Connect the TIDA-010997 BoosterPack to the LP-MSPM0G5187 LaunchPad
   - Ensure the boosterpack is properly seated and connections are secure

2. **Operation**
   - On startup, the application initializes the sensor, peripherals, and TinyEngine™ NPU.
   - The BMI270 IMU collects accelerometer data and stores it in its FIFO buffer.
   - When the FIFO watermark is reached (FE_FRAME_SIZE samples), an interrupt triggers data acquisition via SPI.
   - The application parses the raw FIFO byte stream into structured x/y/z acceleration sample arrays.
   - Feature extraction is performed on each axis, and features are stored in a sliding window.
   - Every frame, the AI model runs inference using the accumulated features.
   - Post-processing applies temporal voting: a fall is confirmed when at least 5 out of the last 8 inferences indicate a fall.
   - The LED glows red if a fall is detected, otherwise it glows green indicating normal activity (ADL).

3. **Running the Example**
   - Compile, load, and run the application on your MSPM0 device.
   - The system will continuously monitor vibration and indicate detected fault classes via LEDs.

4. **Live Preview**
   - Open CCStudio™ Edge AI Studio.
   - Open the `Time Series Classification` example.
   - Click on the Live Preview tab.
   - Make sure the application is running on the device and click on connect to device.
   - Click on start preview to view the predicted class and output values on the GUI.
   - Make sure that the correct baudrate (115200) and COM ports are selected before connecting to the device.

## Software Details

- **Feature Extraction**: Vibration data collected from BMI270 accelerometer, present on TIDA-010997 EdgeAI BoosterPack, via SPI.
- **AI Inference**: Runs a pre-trained model using TVM runtime.
- **Interrupt Handling**: Efficient data collection via GPIO interrupts.
- **LED Indication**: Classification result is indicated by toggling onboard LEDs.
- **AI Model**: Model details are available on [Model Zoo](https://github.com/TexasInstruments/tinyml-tensorlab/tree/r1.3/tinyml-modelzoo)

## Notes

- Ensure the TIDA-010997 EdgeAI BoosterPack is properly powered and connected.
- Adjust LED and GPIO pin assignments as needed for your hardware.
- The model uses frequency-domain features extracted from accelerometer data.
- This example requires MSPM0G5187 which includes the TinyEngine™ NPU hardware accelerator.

## References

- MSPM0G5187 Technical Reference Manual [Link](https://www.ti.com/product/MSPM0G5187)
- [TI Neural Network Compiler Guide](https://software-dl.ti.com/mctools/nnc/mcu/users_guide/)
- TI Model Training Guide: [tinyml-tensorlab](https://github.com/TexasInstruments/tinyml-tensorlab/tree/main)
- EdgeAI Software Guide: SDK_INSTALL_DIR/docs/english/middleware/edgeAI/MSPM0_EdgeAI_User_Guide.html
- For more information on TIDA-010997 EdgeAI BoosterPack and BMI270 IMU sensor, refer to the schematic and datasheet:
    - [TIDA-010997 Schematic](https://www.ti.com/lit/df/slvrc11/slvrc11.pdf?ts=1779166763123)
    - [BMI270 Datasheet](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmi270-ds000.pdf)
