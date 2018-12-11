/*
 * LabADC.cpp
 *
 *  Created on: Oct 1, 2018
 *      Author: kimfl
 */

#include "LabADC.hpp"
#include "LPC17xx.h"

LabADC::LabADC()
{

}

void LabADC::AdcInitBurstMode()
{
    LPC_SC->PCONP |= (1 << 12);         // power on
    LPC_ADC->ADCR |= (1 << 21);         // power on A/D converter
    LPC_SC->PCLKSEL0 &= ~(3 << 24);      // select ADC
    LPC_SC->PCLKSEL0 |= (1 << 24);      // select ADC
    LPC_ADC->ADCR |= (3 << 8);          // bits 8-15; clock is divided by value+1

    LPC_ADC->ADCR &= ~(7 << 24);          // clear start conversion
    LPC_ADC->ADCR |= (1 << 16);         // select burst
}

void LabADC::AdcSelectPin(Pin pin)
{
    switch(pin)
    {
        case k0_25: LPC_PINCON->PINSEL1 &= ~(3 << 18);      // clear pinsel
                    LPC_PINCON->PINSEL1 |= (1 << 18);      // select ADC for P0.25 (AD0.2)
                    LPC_ADC->ADCR |= (1 << 2);          // select AD0.2
                    break;
        case k0_26: LPC_PINCON->PINSEL1 &= ~(3 << 20);      // clear pinsel
                    LPC_PINCON->PINSEL1 |= (1 << 20);      // select ADC for P0.26 (AD0.3)
                    LPC_ADC->ADCR |= (1 << 3);          //
                    break;
        case k1_30: LPC_PINCON->PINSEL3 &= ~(3 << 20);      // clear pinsel
//                    LPC_PINCON->PINSEL3 &= ~(0 << 21);      // clear pinsel
                    LPC_PINCON->PINSEL3 |= (3 << 28);      // select ADC for P1.30 (AD0.4)
                    LPC_ADC->ADCR |= (1 << 4);
                    break;
        case k1_31: LPC_PINCON->PINSEL3 &= ~(3 << 30);      // clear pinsel
//                    LPC_PINCON->PINSEL3 &= ~(0 << 31);      // clear pinsel
                    LPC_PINCON->PINSEL3 |= (3 << 30);      // select ADC for P1.31 (AD0.5)
                    LPC_ADC->ADCR |= (1 << 5);
                    break;
        default: break;
    }
}

float LabADC::ReadAdcVoltageByChannel(uint8_t channel)
{
//    3V/(2^12)
//    LPC_ADC->ADCR &= ~(0xFF | (7 << 24));       // clear channels
//    LPC_ADC->ADCR |= (1 << channel);
//    LPC_ADC->ADCR &= ~(7 << 24);          // clear start conversion
    uint16_t result;
    switch(channel)
    {
        case 0: {
                    result = LPC_ADC->ADDR0 >> 4;
                    return result*(3.3/4096);
                    break;
                }

        case 1: {
                    result = LPC_ADC->ADDR1 >> 4;
                    return result*(3.3/4096);
                    break;
                }
        case 2: {
                    result = LPC_ADC->ADDR2 >> 4;
                    return result*(3.3/4096);
                    break;
                }
        case 3: {
                    result = LPC_ADC->ADDR3 >> 4;
                    return result*(3.3/4096);
                    break;
                }

        case 4: {
                    result = LPC_ADC->ADDR4 >> 4;
                    return result*(3.3/4096);
                    break;
                }
        case 5: {
                    result = LPC_ADC->ADDR5 >> 4;
                    return result*(3.3/4096);
                    break;
                }

        case 6: {
                    result = LPC_ADC->ADDR6 >> 4;
                    return result*(3.3/4096);
                    break;
                }

        case 7: {
                    result = LPC_ADC->ADDR7 >> 4;
                    return result*(3.3/4096);
                    break;
                }

        default: return 0;
    }
}
