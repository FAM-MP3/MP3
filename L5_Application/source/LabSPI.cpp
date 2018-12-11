/*
 * LabSPI.cpp
 *
 *  Created on: Sep 25, 2018
 *      Author: kimfl
 */
#include <stdio.h>
#include "FreeRTOS.h"
#include "LabSPI.hpp"
#include "LPC17xx.h"
#include "semphr.h"
#include "spi_sem.h"

LabSPI::LabSPI()
{
    LPC_SC->PCLKSEL0 &= ~(3 << 20);             // clear clock
    LPC_SC->PCLKSEL0 &= ~(3 << 30);
    LPC_PINCON->PINSEL0 &= ~(3 << 14);          // clear pinsel0
    LPC_PINCON->PINSEL0 &= ~(0xF << 2);

}

LabSPI::~LabSPI()
{
    LPC_SC->PCLKSEL0 &= ~(3 << 20);             // clear clock
    LPC_SC->PCLKSEL0 &= ~(3 << 30);
    LPC_PINCON->PINSEL0 &= ~(3 << 14);          // clear pinsel0
    LPC_PINCON->PINSEL0 &= ~(0xF << 2);
}

bool LabSPI::initialize(uint8_t data_size_select, FrameModes format, uint8_t divide)
{
    if((divide%2) != 0 || divide == 0) // divide must not be 0 or odd
        return false;
    if((data_size_select < 4) || (data_size_select > 16)) // data_size_select must be within [4, 16]
        return false;

    LPC_SC->PCONP |= (1 << 10);                 // power on; 1 = reset, 0 = on
    LPC_SC->PCLKSEL0 |= (1 << 20);              // set clock
    LPC_PINCON->PINSEL0 |= (2 << 14);           // set SCK1 to 10
    LPC_PINCON->PINSEL0 |= (0xA << 16);
//    LPC_PINCON->PINSEL1 &= ~(0xF << 2);         // set MISO MOSI

    LPC_SSP1->CR0 = data_size_select - 1;
    LPC_SSP1->CR1 = (1 << 1);                   // master mode
    LPC_SSP1->CPSR = divide;                    // clk / 4 recommended

    switch(format)
    {
        case SPI: LPC_SSP1->CR0 &= ~(3 << 4);   // reset FRF
                break;
        case TI: LPC_SSP1->CR0 |= (1 << 4);     // set FRF to 01
                break;
        case Microwire: LPC_SSP1->CR0 |= (2 << 4);  // set FRF to 10
                        break;
        default: return false;
    }

    return true;
}

uint8_t LabSPI::transfer(uint8_t send)
{
    spi1_lock();
    LPC_SSP1->DR = send;
    while(LPC_SSP1->SR & (1 << 4));     // wait until SSP is busy
    spi1_unlock();
    return LPC_SSP1->DR;
}


