#include "ADC.h"
#include "stm32c0xx.h" // update this if your STM32 family header is named differently

void adc_init(void) {
    // enable clock access to adc
    RCC->APBENR2 |= RCC_APBENR2_ADCEN;

    // configure adc clock source (optional, depends on default configuration)

    // ensure adc is disabled before making changes
    if ((ADC1->CR & ADC_CR_ADEN) != 0) {
        ADC1->CR |= ADC_CR_ADDIS;
        while ((ADC1->CR & ADC_CR_ADEN) != 0); // wait for disable
    }

    // enable the adc voltage regulator
    ADC1->CR |= ADC_CR_ADVREGEN;

    // short delay for the voltage regulator to stabilize
    for (volatile int i = 0; i < 1000; i++); // crude delay

    // set 12-bit resolution as default
    adc_setWidth(12);

    // set default channel to channel 0 (PA0)
    adc_setChannel(0);

    // enable adc
    ADC1->CR |= ADC_CR_ADEN;

    // wait for adc to be ready
    while (!(ADC1->ISR & ADC_ISR_ADRDY));
}

void adc_setChannel(unsigned int chNum) {
    // configure channel in the adc channel selection register
    ADC1->CHSELR = (1 << chNum);
}

void adc_setWidth(int widthCode) {
    // clear resolution bits (bits 3 and 4 in CFGR1)
    ADC1->CFGR1 &= ~(ADC_CFGR1_RES);

    // set resolution
    if (widthCode == 12) {
        // 00 = 12-bit resolution
        ADC1->CFGR1 |= (0 << ADC_CFGR1_RES_Pos);
    } else if (widthCode == 10) {
        ADC1->CFGR1 |= (1 << ADC_CFGR1_RES_Pos);
    } else if (widthCode == 8) {
        ADC1->CFGR1 |= (2 << ADC_CFGR1_RES_Pos);
    } else if (widthCode == 6) {
        ADC1->CFGR1 |= (3 << ADC_CFGR1_RES_Pos);
    }
    // else leave default
}

int adc_getValue(void) {
    // start conversion
    ADC1->CR |= ADC_CR_ADSTART;

    // wait for conversion to complete
    while (!(ADC1->ISR & ADC_ISR_EOC));

    // read and return the converted value
    return (int) ADC1->DR;
}
