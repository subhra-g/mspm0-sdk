/*
* Copyright (c) 2026, Texas Instruments Incorporated
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

/* Includes */
#include "feature_extraction.h"
#include "data_collection.h"
#include "buffer_manager.h"
#include "common.h"

/* Data for SPI to transmit and receive */
uint16_t gTxData = 0xABAB;
#define SPI_PACKET_SIZE         (1)

/* Audio Samples Read count */
volatile uint32_t samples_count = 0;

volatile uint32_t adc_overflow = 0;

#define HWREG(x)                            (*((volatile uint32_t *)(x)))

void clockConfig(void)
{
    HWREG(0x400B1404)  = 0x81071407;
    DL_SYSCTL_setSYSOSCFreq(DL_SYSCTL_SYSOSC_FREQ_4M);
    DL_SYSCTL_setSYSOSCFreq(DL_SYSCTL_SYSOSC_FREQ_BASE);

    /*********************     SYSPLL Configurations      ***************************/
    SYSCTL->SOCLOCK.SYSPLLCFG0  = ( SYSCTL_SYSPLLCFG0_ENABLECLK0_ENABLE|SYSCTL_SYSPLLCFG0_ENABLECLK1_ENABLE |0x0 <<SYSCTL_SYSPLLCFG0_RDIVCLK1_OFS | 0x0 <<SYSCTL_SYSPLLCFG0_RDIVCLK0_OFS); // SYSPLLCLK0 / 1 enable and Rdiv setting , Actual R divide =2*(Rdiv0 +1)
    DL_Common_updateReg(&SYSCTL->SOCLOCK.SYSPLLCFG0 , SYSCTL_SYSPLLCFG0_SYSPLLREF_SYSOSC, SYSCTL_SYSPLLCFG0_SYSPLLREF_MASK ); // Enable sysosc as SYPLL ref source
    SYSCTL->SOCLOCK.SYSPLLCFG1  = (SYSCTL_SYSPLLCFG1_PDIV_REFDIV1| 0x4 << SYSCTL_SYSPLLCFG1_QDIV_OFS);      // Pdiv and qdiv settings fvco = (syspll _refclk * (qdiv+1))/(2^(pdiv))

    /*********************     TRim values for 32- 48 MHz Range  ***************************/
    SYSCTL->SOCLOCK.SYSPLLPARAM0    =   0x05060A;                   //    Load PARAM0 trim setting from TRM SYSPLL Parameter Lookup according to floopin freq
    SYSCTL->SOCLOCK.SYSPLLPARAM1    =   0x1000110;                  //    Load PARAM1 trim setting from TRM SYSPLL Parameter Lookup according to floopin freq
    HWREG(0x400B1450)               =   0x80FA0;                    //    PLL TRIM 0
    HWREG(0x400B1454)               =   0xA28;                      //    PLL TRIM 1

    DL_Common_updateReg(&SYSCTL->SOCLOCK.HSCLKEN,  SYSCTL_HSCLKEN_SYSPLLEN_ENABLE, SYSCTL_HSCLKEN_SYSPLLEN_MASK );         // Enable PLL
    // Need wait PLL Lock
    asm("NOP");
    asm("NOP");
    asm("NOP");
    asm("NOP");
    while (!(DL_SYSCTL_getClockStatus() & DL_SYSCTL_CLK_STATUS_SYSPLL_GOOD));

    SYSCTL->SOCLOCK.HSCLKCFG = SYSCTL_HSCLKCFG_HSCLKSEL_SYSPLL;   // To select either HFCLK or SYSPLL as High speed clock source
    SYSCTL->SOCLOCK.MCLKCFG = (SYSCTL_MCLKCFG_USEHSCLK_ENABLE| 0x210);  // To select either SYSOSC or HSCCLK as MCLK
}



/* Initialize the peripherals for Collecting audio input via SPI MIC or BOOSTXL MIC */
void mic_init(void)
{
    SYSCFG_DL_init();
    clockConfig();

    NVIC_EnableIRQ(ADC12_0_INST_INT_IRQN);
    DL_GPIO_setPins(GPIO_GRP_0_PORT, GPIO_GRP_0_PIN_0_PIN);


}



uint8_t inputOffset = 0;
void ADC12_0_INST_IRQHandler(void)
{
    switch (DL_ADC12_getPendingInterrupt(ADC12_0_INST)) {
        case DL_ADC12_IIDX_MEM0_RESULT_LOADED:
            audio_sample = DL_ADC12_getMemResult(ADC12_0_INST, ADC12_0_ADCMEM_0);
            shift_audio_buffer();
            samples_count++;
            break;
        case DL_ADC12_IIDX_TRIG_OVERFLOW:
            // Entering this else statement is an error
            adc_overflow++;
            break;
        default:
            break;
    }
}
