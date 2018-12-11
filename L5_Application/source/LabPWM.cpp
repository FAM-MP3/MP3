/*
 * LabPWM.cpp
 *
 *  Created on: Oct 2, 2018
 *      Author: kimfl
 */

#include "LabPWM.hpp"
#include "LPC17xx.h"


void LabPWM::PwmSelectAllPins()
{
    LPC_PINCON->PINSEL4 |= (1 << 0);
    LPC_PINCON->PINSEL4 |= (1 << 2);
    LPC_PINCON->PINSEL4 |= (1 << 4);
    LPC_PINCON->PINSEL4 |= (1 << 6);
    LPC_PINCON->PINSEL4 |= (1 << 8);
    LPC_PINCON->PINSEL4 |= (1 << 10);
}

void LabPWM::PwmSelectPin(PWM_PIN pwm_pin_arg)
{
    switch(pwm_pin_arg)
    {
        case k2_0:  LPC_PINCON->PINSEL4 |= (1 << 0);
                    break;
        case k2_1:  LPC_PINCON->PINSEL4 |= (1 << 2);
                    break;
        case k2_2:  LPC_PINCON->PINSEL4 |= (1 << 4);
                    break;
        case k2_3:  LPC_PINCON->PINSEL4 |= (1 << 6);
                    break;
        case k2_4:  LPC_PINCON->PINSEL4 |= (1 << 8);
                    break;
        case k2_5:  LPC_PINCON->PINSEL4 |= (1 << 10);
                    break;
        default:    break;
    }
}

void LabPWM::PwmInitSingleEdgeMode(uint32_t frequency_Hz)
{
    LPC_SC->PCONP |= (1 << 6);      // power on PCPWM1
    LPC_SC->PCLKSEL0 |= (1 << 12);  // select PCLK_PWM1

    LPC_PWM1->PR = 0;              // set PR; 48Mhz/47+1 = 1Mhz
    SetFrequency(frequency_Hz);

    // set single edge mode
    LPC_PWM1->PCR |= (0 << 2);
    LPC_PWM1->PCR |= (0 << 3);
    LPC_PWM1->PCR |= (0 << 4);
    LPC_PWM1->PCR |= (0 << 5);
    LPC_PWM1->PCR |= (0 << 6);

    LPC_PWM1->TCR |= (1 << 0);
    LPC_PWM1->TCR |= (1 << 3);      // enable PWM in TCR

    LPC_PWM1->CTCR &= ~(1 << 0);
    LPC_PWM1->CTCR &= ~(1 << 1);

    LPC_PWM1->MCR |= (1 << 1);      // reset TC when it reaches MR0
//    LPC_PWM1->MCR |= (1 << 4);
//    LPC_PWM1->MCR |= (1 << 7);
//    LPC_PWM1->MCR |= (1 << 10);
//    LPC_PWM1->MCR |= (1 << 13);
//    LPC_PWM1->MCR |= (1 << 16);
//    LPC_PWM1->MCR |= (1 << 19);



    LPC_PWM1->PCR |= (1 << 9);      // enable output for PWM1
    LPC_PWM1->PCR |= (1 << 10);     // enable output for PWM2
    LPC_PWM1->PCR |= (1 << 11);     // enable output for PWM3
    LPC_PWM1->PCR |= (1 << 12);     // enable output for PWM4
    LPC_PWM1->PCR |= (1 << 13);     // enable output for PWM5
    LPC_PWM1->PCR |= (1 << 14);     // enable output for PWM6

    // Set duty cycle to 0
    LPC_PWM1->MR1 = 0;
    LPC_PWM1->LER |= (1 << 1);
    LPC_PWM1->MR2 = 0;
    LPC_PWM1->LER |= (1 << 2);
    LPC_PWM1->MR3 = 0;
    LPC_PWM1->LER |= (1 << 3);
    LPC_PWM1->MR4 = 0;
    LPC_PWM1->LER |= (1 << 4);
    LPC_PWM1->MR5 = 0;
    LPC_PWM1->LER |= (1 << 5);
    LPC_PWM1->MR6 = 0;
    LPC_PWM1->LER |= (1 << 6);
//    LPC_PWM1->LER |= (0x2F << 0);

    LPC_PWM1->CCR = 0;
}

void LabPWM::SetDutyCycle(PWM_PIN pwm_pin_arg, float duty_cycle_percentage)
{
    switch(pwm_pin_arg)
    {
        case k2_0:  LPC_PWM1->MR1 = LPC_PWM1->MR0 * (duty_cycle_percentage);
                    LPC_PWM1->LER |= (1 << 1);
                    break;
        case k2_1:  LPC_PWM1->MR2 = LPC_PWM1->MR0 * (duty_cycle_percentage);
                    LPC_PWM1->LER |= (1 << 2);
                    break;
        case k2_2:  LPC_PWM1->MR3 = LPC_PWM1->MR0 * (duty_cycle_percentage);
                    LPC_PWM1->LER |= (1 << 3);
                    break;
        case k2_3:  LPC_PWM1->MR4 = LPC_PWM1->MR0 * (duty_cycle_percentage);
                    LPC_PWM1->LER |= (1 << 4);
                    break;
        case k2_4:  LPC_PWM1->MR5 = LPC_PWM1->MR0 * (duty_cycle_percentage);
                    LPC_PWM1->LER |= (1 << 5);
                    break;
        case k2_5:  LPC_PWM1->MR6 = LPC_PWM1->MR0 * (duty_cycle_percentage);
                    LPC_PWM1->LER |= (1 << 6);
                    break;
        default:    break;
    }
}

void LabPWM::SetFrequency(uint32_t frequency_Hz)
{
    LPC_PWM1->MR0 = (48000000/(LPC_PWM1->PR + 1))/frequency_Hz;   // PR is set as 47, so the incoming clk is 1Mhz
    LPC_PWM1->LER |= (1 << 0);
}
