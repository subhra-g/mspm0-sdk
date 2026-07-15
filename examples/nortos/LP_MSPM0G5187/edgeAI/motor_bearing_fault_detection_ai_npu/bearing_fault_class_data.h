/*
 * Copyright (c) 2025, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef BEARING_FAULT_CLASS_DATA_H_
#define BEARING_FAULT_CLASS_DATA_H_

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/**s
 * @brief Classes of Bearing Fault data for classification
 */
typedef enum {
    BEARING_FAULT_CLASS_1_NORMAL = 0,
    BEARING_FAULT_CLASS_2_CONTAMINATED,
    BEARING_FAULT_CLASS_3_EROSION,
    BEARING_FAULT_CLASS_4_FLAKING,
    BEARING_FAULT_CLASS_5_NOLUBRICATION,
    BEARING_FAULT_CLASS_6_LOCALIZEDFAULT
} Bearing_Fault_Class_t;

#define ENABLE      1
#define DISABLE     0

/* Class type macro enabled or disabled here */
#define CLASS_1     ENABLE
#define CLASS_2     DISABLE
#define CLASS_3     DISABLE
#define CLASS_4     DISABLE
#define CLASS_5     DISABLE
#define CLASS_6     DISABLE

/**
 * @brief Class 1: Normal Bearing Fault data
 *
 */
#if (CLASS_1)
 extern const int32_t BEARING_FAULT_CLASS_NORMAL[2048*3];
#endif /* CLASS_TYPE */

/**
 * @brief Class 2: Contaminated Bearing Fault data
 *
 */
#if (CLASS_2)
 extern const int32_t BEARING_FAULT_CLASS_CONTAMINATED[2048*3];
#endif /* CLASS_TYPE */

/**
 * @brief Class 3: Erosion Bearing Fault data
 *
 */
#if (CLASS_3)
 extern const int32_t BEARING_FAULT_CLASS_EROSION[2048*3];
#endif /* CLASS_TYPE */

/**
 * @brief Class 4: Flaking Bearing Fault data
 *
 */
#if (CLASS_4)
 extern const int32_t BEARING_FAULT_CLASS_FLAKING[2048*3];
#endif /* CLASS_TYPE */

/**
 * @brief Class 5: No librication Bearing Fault data
 *
 */
#if (CLASS_5)
 extern const int32_t BEARING_FAULT_CLASS_NOLUBRICATION[2048*3];
#endif /* CLASS_TYPE */

/**
 * @brief Class 6: Localized Bearing Fault data
 *
 */
#if (CLASS_6)
 extern const int32_t BEARING_FAULT_CLASS_LOCALIZEDFAULT[2048*3];
#endif /* CLASS_TYPE */

/**
 * @brief Get the current Bearing Fault class data pointer
 *
 * @return Pointer to the currently selected class data array
 */
const int32_t *get_current_bearing_fault_class_data(void);

#endif /* BEARING_FAULT_CLASS_DATA_H_ */
