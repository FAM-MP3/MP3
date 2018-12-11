/*
 * LabGPIO.cpp
 *
 *  Created on: Aug 31, 2018
 *      Author: kimfl
 */

#include <stdio.h>      // printf
#include "LabGPIO.hpp"
#include "LPC17xx.h"

LabGPIO::LabGPIO(uint8_t port, uint8_t pin)
{
    LPC_GPIO_pin = pin;
    LPC_GPIO_port = port;

    uint32_t gpioBases[] = {LPC_GPIO0_BASE, LPC_GPIO1_BASE, LPC_GPIO2_BASE, LPC_GPIO3_BASE, LPC_GPIO4_BASE};
    LPC_GPIO = (LPC_GPIO_TypeDef*) gpioBases[port];

}

void LabGPIO::setAsInput(void)
{
    LPC_GPIO->FIODIR &= ~(1 << LPC_GPIO_pin);
}

void LabGPIO::setAsOutput(void)
{
    LPC_GPIO->FIODIR |=  (1 << LPC_GPIO_pin);
}

void LabGPIO::setDirection(bool output)
{
    if (output){ // output mode
        LPC_GPIO->FIODIR &= ~(1 << LPC_GPIO_pin);
    }
    else{        // input mode
        LPC_GPIO->FIODIR |=  (1 << LPC_GPIO_pin);
    }
}

void LabGPIO::setHigh()
{
    LPC_GPIO->FIOSET =  (1 << LPC_GPIO_pin);
}

void LabGPIO::setLow()
{
    LPC_GPIO->FIOCLR = (1 << LPC_GPIO_pin);
}

void LabGPIO::set(bool high)
{
    if (high) // set high
        setHigh();
    else        // input mode
        setLow();
}

bool LabGPIO::getLevel()
{
    if (LPC_GPIO->FIOPIN & (1 << LPC_GPIO_pin))
        return true;
    else
        return false;
}

LabGPIO::~LabGPIO()
{
    setAsInput();
}
